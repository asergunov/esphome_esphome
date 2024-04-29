#pragma once

#include "esphome/components/spi/spi.h"

#include "esphome/components/display_7segment_base/display.h"

namespace esphome {
namespace max7219 {

class MAX7219Component;

using max7219_writer_t = std::function<void(MAX7219Component &)>;

class MAX7219Component : public display_7segment_base::Display,
                         public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                               spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_1MHZ> {
 public:
  void set_writer(max7219_writer_t &&writer);

  void setup() override;

  void dump_config() override;

  void update() override;

  float get_setup_priority() const override;

  void display();

  void set_intensity(uint8_t intensity);
  void set_num_chips(uint8_t num_chips);
  void set_reverse(bool reverse) { this->reverse_ = reverse; };

  /// Print `str` at the given position.
  uint8_t print(uint8_t pos, const char *str);

 protected:
  void send_byte_(uint8_t a_register, uint8_t data);
  void send_to_all_(uint8_t a_register, uint8_t data);

  uint8_t intensity_{15};     // Intensity of the display from 0 to 15 (most)
  bool intensity_changed_{};  // True if we need to re-send the intensity
  uint8_t num_chips_{1};
  uint8_t *buffer_;
  bool reverse_{false};
  optional<max7219_writer_t> writer_{};
};

}  // namespace max7219
}  // namespace esphome
