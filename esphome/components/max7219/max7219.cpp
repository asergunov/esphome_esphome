#include "max7219.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace max7219 {

static const char *const TAG = "max7219";

static const uint8_t MAX7219_REGISTER_NOOP = 0x00;
static const uint8_t MAX7219_REGISTER_DECODE_MODE = 0x09;
static const uint8_t MAX7219_REGISTER_INTENSITY = 0x0A;
static const uint8_t MAX7219_REGISTER_SCAN_LIMIT = 0x0B;
static const uint8_t MAX7219_REGISTER_SHUTDOWN = 0x0C;
static const uint8_t MAX7219_REGISTER_TEST = 0x0F;

float MAX7219Component::get_setup_priority() const { return setup_priority::PROCESSOR; }
void MAX7219Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MAX7219...");
  this->spi_setup();
  this->buffer_ = new uint8_t[this->num_chips_ * 8];  // NOLINT
  for (uint8_t i = 0; i < this->num_chips_ * 8; i++)
    this->buffer_[i] = 0;

  // let's assume the user has all 8 digits connected, only important in daisy chained setups anyway
  this->send_to_all_(MAX7219_REGISTER_SCAN_LIMIT, 7);
  // let's use our own ASCII -> led pattern encoding
  this->send_to_all_(MAX7219_REGISTER_DECODE_MODE, 0);
  this->send_to_all_(MAX7219_REGISTER_INTENSITY, this->intensity_);
  this->display();
  // power up
  this->send_to_all_(MAX7219_REGISTER_TEST, 0);
  this->send_to_all_(MAX7219_REGISTER_SHUTDOWN, 1);
}
void MAX7219Component::dump_config() {
  ESP_LOGCONFIG(TAG, "MAX7219:");
  ESP_LOGCONFIG(TAG, "  Number of Chips: %u", this->num_chips_);
  ESP_LOGCONFIG(TAG, "  Intensity: %u", this->intensity_);
  LOG_PIN("  CS Pin: ", this->cs_);
  LOG_UPDATE_INTERVAL(this);
}

void MAX7219Component::display() {
  for (uint8_t i = 0; i < 8; i++) {
    this->enable();
    for (uint8_t j = 0; j < this->num_chips_; j++) {
      if (reverse_) {
        this->send_byte_(8 - i, buffer_[(num_chips_ - j - 1) * 8 + i]);
      } else {
        this->send_byte_(8 - i, buffer_[j * 8 + i]);
      }
    }
    this->disable();
  }
}
void MAX7219Component::send_byte_(uint8_t a_register, uint8_t data) {
  this->write_byte(a_register);
  this->write_byte(data);
}
void MAX7219Component::send_to_all_(uint8_t a_register, uint8_t data) {
  this->enable();
  for (uint8_t i = 0; i < this->num_chips_; i++)
    this->send_byte_(a_register, data);
  this->disable();
}
void MAX7219Component::update() {
  if (this->intensity_changed_) {
    this->send_to_all_(MAX7219_REGISTER_INTENSITY, this->intensity_);
    this->intensity_changed_ = false;
  }
  for (uint8_t i = 0; i < this->num_chips_ * 8; i++)
    this->buffer_[i] = 0;
  if (this->writer_.has_value())
    (*this->writer_)(*this);
  this->display();
}
uint8_t MAX7219Component::print_(uint8_t start_pos, const char *str) {
  uint8_t pos = start_pos;
  for (; str && *str;) {
    if (*str == '.') {
      if (pos != start_pos)
        pos--;
      this->buffer_[pos] |= 0b10000000;
    } else {
      if (pos >= this->num_chips_ * 8) {
        ESP_LOGE(TAG, "MAX7219 String is too long for the display!");
        break;
      }
      str = char_to_segments_(str, this->buffer_[pos]);
    }
    pos++;
  }
  return pos - start_pos;
}
void MAX7219Component::set_writer(max7219_writer_t &&writer) { this->writer_ = writer; }
void MAX7219Component::set_intensity(uint8_t intensity) {
  intensity &= 0xF;
  if (intensity != this->intensity_) {
    this->intensity_changed_ = true;
    this->intensity_ = intensity;
  }
}
void MAX7219Component::set_num_chips(uint8_t num_chips) { this->num_chips_ = num_chips; }

}  // namespace max7219
}  // namespace esphome
