
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace tea5767 {

class Tea5767Component : public i2c::I2CDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void seek_up();
  void seek_down();

  enum SSL {
    SSL_LOW = 0b01,   // ADC = 5
    SSL_MID = 0b10,   // ADC = 7
    SSL_HIGH = 0b11,  // ADC = 10
  };

  enum CF {
    CF_13MHz = 0b0,
    CF_32768Hz = 0b1,
    // CF_6500kHz = 0b10,
  };

  enum DTC {
    DTC_50us = 0,
    DTC_75us = 1,
  };

  enum Band {
    Japanese = 1,
    Other = 0,
    US = Other,
    Europe = Other,
  };

  void set_search_stop_level(SSL level) { config_.set<Config::SearchStopLevel>(level); }
  void set_clock_frequency(CF value) { config_.set<Config::XTAL>(value); }
  void set_dtc(DTC value) { config_.set<Config::DeEmphasisTimeConstant>(value); }
  void set_band(Band value) { config_.set<Config::BandLimits>(value); }
  void set_external_pll(bool value = true) { config_.set<Config::PLLREF>(value); }

 protected:
  template<typename DATA, uint8_t INDEX> struct Byte {
    static uint8_t read(const DATA &data) { return data[INDEX]; }
    static void write(DATA &data, uint8_t value) { data[INDEX] = value; }

    static const uint8_t &ref(const DATA &data) { return data[INDEX]; }
    static uint8_t &ref(DATA &data) { return data[INDEX]; }
  };

  template<typename DATA, typename BYTE, uint8_t MSB, uint8_t LSB, typename VALUE_TYPE = uint8_t> struct Field {
    using value_type = VALUE_TYPE;
    using byte_type = BYTE;
    using data_type = DATA;

    static constexpr uint8_t MASK = uint8_t(((uint16_t(1) << (MSB - LSB + 1)) - 1) << LSB);

    static value_type read(const data_type &data) {
      return static_cast<value_type>((byte_type::read(data) & MASK) >> LSB);
    }
    static void write(data_type &data, const value_type &value) {
      byte_type::write(data, (byte_type::read(data) & ~MASK) | ((value << LSB) & MASK));
    }
  };
  template<typename DATA, typename BYTE, uint8_t BIT, typename VALUE_TYPE = bool>
  using Bit = Field<DATA, BYTE, BIT, BIT, VALUE_TYPE>;

  template<typename DATA> struct PLL {
    using data_type = DATA;
    using PLL_HI = Field<data_type, typename data_type::Byte1, 5, 0, uint16_t>;
    using PLL_LO = Field<data_type, typename data_type::Byte2, 8, 0, uint16_t>;
    using value_type = uint16_t;
    static value_type read(const data_type &data) { return (PLL_HI::read(data) << 8) | PLL_LO::read(data); }
    static void write(data_type &data, const value_type value) {
      PLL_HI::write(data, value >> 8);
      PLL_LO::write(data, value & 0xff);
    }
  };

  // template<typename PLL> struct Frequency {
  //   using pll_type = PLL;
  //   using value_type = float;
  //   using data_type = typename pll_type::data_type;
  //   static value_type read(const data_type &data) {
  //     return (pll_type::read(data) * 32.768 / 4
  //             //- HightSideInjectionLO::read(data) ? 225 : -225
  //             ) /
  //            1000;
  //   }
  //   static void write(data_type &data, const value_type freq) {
  //     pll_type::write(data, (4 * (freq * 1000
  //                                 //+ HightSideInjectionLO::read(data) ? 225 : -225
  //                                 )) /
  //                               32.768);
  //   }
  // };

  struct Config : std::array<uint8_t, 5> {
    template<uint8_t INDEX> using Byte = Byte<Config, INDEX>;
    using Byte1 = Byte<0>;
    using Byte2 = Byte<1>;
    using Byte3 = Byte<2>;
    using Byte4 = Byte<3>;
    using Byte5 = Byte<4>;
    template<typename BYTE, uint8_t MSB, uint8_t LSB, typename VALUE_TYPE = uint8_t>
    using Field = Field<Config, BYTE, MSB, LSB, VALUE_TYPE>;
    template<typename BYTE, uint8_t BIT, typename VALUE_TYPE = bool> using Bit = Bit<Config, BYTE, BIT, VALUE_TYPE>;

    using Mute = Bit<Byte1, 7>;
    using SearchMode = Bit<Byte1, 6>;
    using SearchUp = Bit<Byte3, 7>;
    using SearchStopLevel = Field<Byte3, 6, 5, SSL>;
    using HightSideInjectionLO = Bit<Byte3, 4>;
    using ForceMono = Bit<Byte3, 3>;
    using MuteLeft = Bit<Byte3, 2>;
    using MuteRight = Bit<Byte3, 1>;
    using SWP1 = Bit<Byte3, 0>;
    using SWP2 = Bit<Byte4, 7>;
    using Standby = Bit<Byte4, 6>;
    using BandLimits = Bit<Byte4, 5, Band>;
    using XTAL = Bit<Byte4, 4, CF>;
    using SoftMute = Bit<Byte4, 3>;
    using HighCutControl = Bit<Byte4, 2>;
    using StereoNoiseCancelation = Bit<Byte4, 1>;
    /**
     * if si = true then pin SWPORT1 is output for the ready
     * flag; if si = false then pin SWPORT1 is software programmable port 1
     */
    using SearchIndicator = Bit<Byte4, 0>;
    using PLLREF = Bit<Byte5, 7>;
    using DeEmphasisTimeConstant = Bit<Byte5, 6, DTC>;

    template<typename T> typename T::value_type get() const { return T::read(*this); }
    template<typename T> void set(const typename T::value_type &value, bool set_dirty = true) {
      T::write(*this, value);
      dirty_ = dirty_ || set_dirty;
    }

    uint16_t if_conter_period_us() { return get<XTAL>() ? 15625 : 15754; }
    float if_counter_resolution_hz() { return get<XTAL>() ? 4096 : 4062.5; }
    uint16_t nref() {
      if (get<PLLREF>())
        return 130;
      return get<XTAL>() ? 130 : 260;
    }

    float pll_to_freq(uint16_t pll) const {
      const auto f_if = get<HightSideInjectionLO>() ? 225e3f : -225e3f;
      const auto f_ref = get<XTAL>() ? 32768.f : 50000.f;
      return pll * f_ref / 4.0f - f_if;
    }

    uint16_t freq_to_pll(float freq) const {
      const auto f_if = get<HightSideInjectionLO>() ? 225e3f : -225e3f;
      const auto f_ref = get<XTAL>() ? 32768.f : 50000.f;
      return 4.0f * (freq + f_if) / f_ref;
    }

    float freq() const { return pll_to_freq(get<PLL<Config>>()); }

    bool dirty_ = false;
  };

  struct Status : std::array<uint8_t, 5> {
    template<uint8_t INDEX> using Byte = Byte<Status, INDEX>;
    using Byte1 = Byte<0>;
    using Byte2 = Byte<1>;
    using Byte3 = Byte<2>;
    using Byte4 = Byte<3>;
    using Byte5 = Byte<4>;
    template<typename BYTE, uint8_t MSB, uint8_t LSB, typename VALUE_TYPE = uint8_t>
    using Field = Field<Status, BYTE, MSB, LSB, VALUE_TYPE>;
    template<typename BYTE, uint8_t BIT, typename VALUE_TYPE = bool> using Bit = Bit<Status, BYTE, BIT, VALUE_TYPE>;

    using ReadyFlag = Bit<Byte1, 7>;
    using BandLimitFlag = Bit<Byte1, 6>;
    using Stereo = Bit<Byte3, 7>;
    using IF = Field<Byte3, 6, 0>;
    using LEV = Field<Byte4, 7, 4>;
    using CI = Field<Byte4, 3, 1>;

    template<typename T> typename T::value_type get() const { return T::read(*this); }
  };

  void write_();
  void read_();

  Config config_;
  Status status_;
};

}  // namespace tea5767
}  // namespace esphome
