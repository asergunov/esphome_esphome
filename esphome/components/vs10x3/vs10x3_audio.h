#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"

#include <VS1053.h>

namespace esphome {
namespace vs10x3 {

class Vs10x3AudioComponent : public Component,
                             public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                                   spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_200KHZ> {
 public:
  void setup() override;
  void dump_config() override;

  void set_dcs_pin(uint8_t pin) { this->dcs_pin_ = pin; }
  void set_cs_pin(uint8_t pin) { this->cs_pin_ = pin; }
  void set_dreq_pin(uint8_t pin) { this->dreq_pin_ = pin; }

 protected:
  std::unique_ptr<VS1053> vs1053_;
  uint8_t dcs_pin_;
  uint8_t cs_pin_;
  uint8_t dreq_pin_;
};

}  // namespace vs10x3
}  // namespace esphome
