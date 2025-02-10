#pragma once

#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"

#include "registers.h"

class VS1053;  // TODO: to remove

namespace esphome {
namespace vs10x3 {

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

class Vs10x3AudioComponent
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

  void set_xtal_frequency(uint32_t value) { this->clki_ = this->xtali_ = value; }
  void set_clock_multiplier(uint8_t mult, uint8_t add) {
    const auto new_clockf =
        (clockf_ & 0b11111000000000000) | (uint16_t(mult & 0b111) << 13) | (uint16_t(add & 0b11) << 11);
    if (new_clockf != this->clockf_) {
      clockf_ = new_clockf;
      if (!in_hw_reset_) {
        write_register_(SCI_CLOCKF, new_clockf);
        this->clki_ = this->xtali_ * (2 + ((clockf_ >> 13) & 0b111)) / 2;
      }
    }
  }

  DataDevice &get_data_device() { return data_device_; }
  VS1053 *get_decoder() const { return nullptr; }

 protected:
  void wait_sci_ready_();
  void write_register_(Register r, uint16_t value);
  uint16_t read_register_(Register r);
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

  unsigned long last_command_worst_duration_ = 0;
  unsigned long last_command_sent_micros_ = 0;

  InterruptData interrupt_data_;
  bool in_hw_reset_ = true;
  vs1xxx_registers::SS_VER version_ = 0xff;
};

}  // namespace vs10x3
}  // namespace esphome
