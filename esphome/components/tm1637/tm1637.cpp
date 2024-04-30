#include "tm1637.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tm1637 {

static const char *const TAG = "display.tm1637";
const uint8_t TM1637_CMD_DATA = 0x40;  //!< Display data command
const uint8_t TM1637_CMD_CTRL = 0x80;  //!< Display control command
const uint8_t TM1637_CMD_ADDR = 0xc0;  //!< Display address command

// Data command bits
const uint8_t TM1637_DATA_WRITE = 0x00;          //!< Write data
const uint8_t TM1637_DATA_READ_KEYS = 0x02;      //!< Read keys
const uint8_t TM1637_DATA_AUTO_INC_ADDR = 0x00;  //!< Auto increment address
const uint8_t TM1637_DATA_FIXED_ADDR = 0x04;     //!< Fixed address

void TM1637Display::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TM1637...");

  this->clk_pin_->setup();               // OUTPUT
  this->clk_pin_->digital_write(false);  // LOW
  this->dio_pin_->setup();               // OUTPUT
  this->dio_pin_->digital_write(false);  // LOW

  this->display();
}
void TM1637Display::dump_config() {
  ESP_LOGCONFIG(TAG, "TM1637:");
  ESP_LOGCONFIG(TAG, "  Intensity: %d", this->intensity_);
  ESP_LOGCONFIG(TAG, "  Inverted: %d", this->inverted_);
  ESP_LOGCONFIG(TAG, "  Length: %d", this->length_);
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
  LOG_PIN("  DIO Pin: ", this->dio_pin_);
  LOG_UPDATE_INTERVAL(this);
}

#ifdef USE_BINARY_SENSOR
void TM1637Display::loop() {
  uint8_t val = this->get_keys();
  for (auto *tm1637_key : this->tm1637_keys_)
    tm1637_key->process(val);
}

uint8_t TM1637Display::get_keys() {
  this->start_();
  this->send_byte_(TM1637_CMD_DATA | TM1637_DATA_READ_KEYS);
  this->start_();
  uint8_t key_code = read_byte_();
  this->stop_();
  if (key_code != 0xFF) {
    // Invert key_code:
    //    Bit |  7  6  5  4  3  2  1  0
    //  ------+-------------------------
    //   From | S0 S1 S2 K1 K2 1  1  1
    //     To | S0 S1 S2 K1 K2 0  0  0
    key_code = ~key_code;
    // Shift bits to:
    //    Bit | 7  6  5  4  3  2  1  0
    //  ------+------------------------
    //     To | 0  0  0  0  K2 S2 S1 S0
    key_code = (uint8_t) ((key_code & 0x80) >> 7 | (key_code & 0x40) >> 5 | (key_code & 0x20) >> 3 | (key_code & 0x08));
  }
  return key_code;
}
#endif

void TM1637Display::update() {
  for (uint8_t &i : this->buffer_)
    i = 0;
  if (this->writer_.has_value())
    (*this->writer_)(*this);
  this->display();
}

float TM1637Display::get_setup_priority() const { return setup_priority::PROCESSOR; }
void TM1637Display::bit_delay_() { delayMicroseconds(100); }
void TM1637Display::start_() {
  this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->bit_delay_();
}

void TM1637Display::stop_() {
  this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
  bit_delay_();
  this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
  bit_delay_();
  this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
  bit_delay_();
}

void TM1637Display::display() {
  ESP_LOGVV(TAG, "Display %02X%02X%02X%02X", buffer_[0], buffer_[1], buffer_[2], buffer_[3]);

  // Write DATA CMND
  this->start_();
  this->send_byte_(TM1637_CMD_DATA);
  this->stop_();

  // Write ADDR CMD + first digit address
  this->start_();
  this->send_byte_(TM1637_CMD_ADDR);

  // Write the data bytes
  if (this->inverted_) {
    for (int8_t i = this->length_ - 1; i >= 0; i--) {
      this->send_byte_(this->buffer_[i]);
    }
  } else {
    for (auto b : this->buffer_) {
      this->send_byte_(b);
    }
  }

  this->stop_();

  // Write display CTRL CMND + brightness
  this->start_();
  this->send_byte_(TM1637_CMD_CTRL + ((this->intensity_ & 0x7) | (this->on_ ? 0x08 : 0x00)));
  this->stop_();
}
bool TM1637Display::send_byte_(uint8_t b) {
  uint8_t data = b;
  for (uint8_t i = 0; i < 8; i++) {
    // CLK low
    this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);
    this->bit_delay_();
    // Set data bit
    if (data & 0x01) {
      this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
    } else {
      this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
    }

    this->bit_delay_();
    // CLK high
    this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
    this->bit_delay_();
    data = data >> 1;
  }
  // Wait for acknowledge
  // CLK to zero
  this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();
  // CLK to high
  this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();
  uint8_t ack = this->dio_pin_->digital_read();
  if (ack == 0) {
    this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
  }

  this->bit_delay_();
  this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->bit_delay_();

  return ack;
}

uint8_t TM1637Display::read_byte_() {
  uint8_t retval = 0;
  // Prepare DIO to read data
  this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();
  // Data is shifted out by the TM1637 on the CLK falling edge
  for (uint8_t bit = 0; bit < 8; bit++) {
    this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
    this->bit_delay_();
    // Read next bit
    retval <<= 1;
    if (this->dio_pin_->digital_read()) {
      retval |= 0x01;
    }
    this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);
    this->bit_delay_();
  }
  // Return DIO to output mode
  // Dummy ACK
  this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->bit_delay_();
  this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();
  this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->bit_delay_();
  this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();
  return retval;
}

uint8_t TM1637Display::print_(uint8_t start_pos, const char *str) {
  // ESP_LOGV(TAG, "Print at %d: %s", start_pos, str);
  uint8_t pos = start_pos;
  bool use_dot = false;
  for (; str && *str != '\0';) {
    uint8_t data = UNKNOWN_CHAR;
    const auto is_dot = *str == '.';
    str = char_to_segments_(str, data);

    // Remap segments, for compatibility with MAX7219 segment definition which is
    // XABCDEFG, but TM1637 is // XGFEDCBA
    if (this->inverted_) {
      // XABCDEFG > XGCBAFED
      data = ((data & 0x80) || use_dot ? 0x80 : 0) |  // no move X
             ((data & 0x40) ? 0x8 : 0) |              // A
             ((data & 0x20) ? 0x10 : 0) |             // B
             ((data & 0x10) ? 0x20 : 0) |             // C
             ((data & 0x8) ? 0x1 : 0) |               // D
             ((data & 0x4) ? 0x2 : 0) |               // E
             ((data & 0x2) ? 0x4 : 0) |               // F
             ((data & 0x1) ? 0x40 : 0);               // G
    } else {
      // XABCDEFG > XGFEDCBA
      data = ((data & 0x80) ? 0x80 : 0) |  // no move X
             ((data & 0x40) ? 0x1 : 0) |   // A
             ((data & 0x20) ? 0x2 : 0) |   // B
             ((data & 0x10) ? 0x4 : 0) |   // C
             ((data & 0x8) ? 0x8 : 0) |    // D
             ((data & 0x4) ? 0x10 : 0) |   // E
             ((data & 0x2) ? 0x20 : 0) |   // F
             ((data & 0x1) ? 0x40 : 0);    // G
    }
    use_dot = is_dot;
    if (use_dot) {
      if ((!this->inverted_) && (pos != start_pos)) {
        this->buffer_[pos - 1] |= 0b10000000;
      }
    } else {
      if (pos >= 6) {
        ESP_LOGE(TAG, "String is too long for the display!");
        break;
      }
      this->buffer_[pos++] = data;
    }
  }
  return pos - start_pos;
}

}  // namespace tm1637
}  // namespace esphome
