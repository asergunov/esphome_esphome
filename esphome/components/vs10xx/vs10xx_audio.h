#pragma once

#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"

#include "registers.h"

namespace esphome {
namespace vs10xx {

enum ModeBits {
  SM_DIFF = 1,
  SM_SETTOZERO = 1 << 1,
  SM_RESET = 1 << 2,
  SM_OUTOFWAV = 1 << 3,
  SM_PDOWN = 1 << 4,
  SM_TESTS = 1 << 5,
  SM_STREAM = 1 << 6,
  SM_SETTOZERO2 = 1 << 7,
  SM_DACT = 1 << 8,
  SM_SDIORD = 1 << 9,
  SM_SDISHARE = 1 << 10,
  SM_SDINEW = 1 << 11,
  SM_ADPCM = 1 << 12,
  SM_ADPCM_HP = 1 << 13,
  SM_LINE_IN = 1 << 14,
};

enum Register {
  SCI_MODE,
  SCI_STATUS,
  SCI_BASS,
  SCI_CLOCKF,
  SCI_DECODE_TIME,
  SCI_AUDATA,
  SCI_WRAM,
  SCI_WRAMADDR,
  SCI_HDAT0,
  SCI_HDAT1,
  SCI_AIADDR,
  SCI_VOL,
  SCI_AICTRL0,
  SCI_AICTRL1,
  SCI_AICTRL2,
  SCI_AICTRL3,
  SCI_Count,
};

struct Delay {
  unsigned long clki_ = 0;
  unsigned long xtali_ = 0;
  constexpr Delay(unsigned long clki = 0, unsigned long xtali = 0) noexcept : clki_{clki}, xtali_{xtali} {}
  static constexpr Delay clki(unsigned long x) { return Delay{x, 0}; }
  static constexpr Delay xtali(unsigned long x) { return Delay{0, x}; }
};

static constexpr std::array<Delay, static_cast<uint8_t>(Register::SCI_Count)> registerWriteWorstDelay{
    Delay::clki(70),      // SCI_MODE
    Delay::clki(40),      // SCI_STATUS
    Delay::clki(2100),    // SCI_BASS
    Delay::xtali(11000),  // SCI_CLOCKF
    Delay::clki(40),      // SCI_DECODE_TIME
    Delay::clki(3200),    // SCI_AUDATA
    Delay::clki(80),      // SCI_WRAM
    Delay::clki(80),      // SCI_WRAMADDR
    {},                   // SCI_HDAT0
    {},                   // SCI_HDAT1
    Delay::clki(3200),    // SCI_AIADDR
    Delay::clki(2100),    // SCI_VOL
    Delay::clki(50),      // SCI_AICTRL0
    Delay::clki(50),      // SCI_AICTRL1
    Delay::clki(50),      // SCI_AICTRL2
    Delay::clki(50),      // SCI_AICTRL3
};

class Vs10xxAudioComponent
    : public Component,
      public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                            static_cast<spi::SPIDataRate>(12000000 / 7)  // Worst case scenatio with miltiplier 1 for
                                                                         // reads: 12Mhz/7
                            > {
 public:
  using DataSpiDevice = spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                       static_cast<spi::SPIDataRate>(12000000 / 4)  // Worst case scenatio with
                                                                                    // miltiplier 1 for writes: 12Mhz/4
                                       >;
  class DataDevice : public DataSpiDevice {
   public:
    void dump_config();
    void set_spi_parent(spi::SPIComponent *parent) { DataSpiDevice::set_spi_parent(parent); }
    void set_cs_pin(GPIOPin *cs) { DataSpiDevice::set_cs_pin(cs); }
    using DataSpiDevice::data_rate_;
  };

  struct InterruptData {
    volatile bool dreq_is_active_ = false;
    volatile bool dreq_was_active_ = false;
  };

  void setup() override;
  void dump_config() override;
  void loop() override;

  void set_dreq_pin(InternalGPIOPin *pin) { this->dreq_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }

  void set_xtal_frequency(uint32_t value) { this->xtali_ = value; }
  void set_clock_multiplier(uint8_t mult, uint8_t add) {
    clockf_ << vs1xxx_registers::SC_MULT(mult) << vs1xxx_registers::SC_ADD(add);
    if (!in_hw_reset_) {
      write_register_(SCI_CLOCKF, clockf_.value());
      update_clki_();
    }
  }

  DataDevice &get_data_device() { return data_device_; }
  bool ready_for_data() const { return !in_hw_reset_ && dreq_is_active_(); }

  size_t write_data(const uint8_t *buf, size_t max_len) {
    data_device_.enable();
    data_device_.write_array(buf, max_len);
    data_device_.disable();

    return max_len;
  }
  class DataWriter {
    Vs10xxAudioComponent &parent_;

   public:
    DataWriter(Vs10xxAudioComponent &parent) : parent_(parent) { parent_.data_device_.enable(); }
    ~DataWriter() { parent_.data_device_.disable(); }

    size_t write(const uint8_t *buf, size_t max_len) {
      size_t wrote = 0;
      while (max_len > wrote && parent_.ready_for_data()) {
        auto chunk = std::min<size_t>(32, max_len - wrote);
        parent_.data_device_.write_array(buf + wrote, chunk);
        wrote += chunk;
      }
      return wrote;
    }
  };
  friend class DataWriter;

  DataWriter make_writer() { return DataWriter{*this}; }

 protected:
  void update_clki_();
  void wait_sci_ready_();

  void write_register_(Register r, uint16_t value);
  uint16_t read_register_(Register r);

  template<typename REG> bool refresh_regiter_(REG &reg) {
    auto val = this->read_register_<REG>();
    if (reg != val) {
      ESP_LOGV("vs10xx", "Register %d has changed from %04X to %04X", REG::REG_ADR, reg, val);
      reg = val;
      return true;
    }
    return false;
  };

  template<typename REG> void write_register_(const REG &value) {
    write_register_(Register(REG::REG_ADR), value.value());
  }
  template<typename REG> REG read_register_() { return REG(read_register_(Register(REG::REG_ADR))); }

  bool handle_reset_();
  bool dreq_is_active_() const { return dreq_pin_->digital_read(); }
  unsigned long xtali_to_micros_(unsigned long xtali) const { return uint64_t(1000000) * xtali / xtali_; }
  unsigned long clki_to_micros_(unsigned long clki) const { return uint64_t(1000000) * clki / clki_; }
  unsigned long delay_to_micros_(const Delay &delay) const {
    return xtali_to_micros_(delay.xtali_) + clki_to_micros_(delay.clki_);
  }

 protected:
  DataDevice data_device_;
  InternalGPIOPin *dreq_pin_ = nullptr;
  GPIOPin *reset_pin_ = nullptr;
  unsigned long hw_reset_start_micros_ = 0;
  uint32_t xtali_ = 12.288e6;
  uint32_t clki_ = 12.288e6;

  vs1xxx_registers::SCI_MODE mode_;
  vs1xxx_registers::SCI_STATUS status_;
  vs1xxx_registers::SCI_CLOCKF clockf_;
  vs1xxx_registers::SCI_DECODE_TIME decode_time_;
  vs1xxx_registers::SCI_AUDATA audata_;
  vs1xxx_registers::SCI_HDAT0 hdat0_;
  vs1xxx_registers::SCI_HDAT1 hdat1_;

  unsigned long last_command_worst_duration_ = 0;
  unsigned long last_command_sent_micros_ = 0;

  InterruptData interrupt_data_;
  bool in_hw_reset_ = true;
  vs1xxx_registers::SS_VER version_ = 0xff;
};

}  // namespace vs10xx
}  // namespace esphome
