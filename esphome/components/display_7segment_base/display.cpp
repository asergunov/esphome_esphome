#include "7segment_display.h"

namespace esphome {
namespace display_7segment_base {

uint8_t Display::print(const char *str) { return this->print(0, str); }
uint8_t Display::printf(uint8_t pos, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[64];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    return this->print(pos, buffer);
  return 0;
}
uint8_t Display::printf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[64];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    return this->print(buffer);
  return 0;
}

uint8_t Display::strftime(uint8_t pos, const char *format, ESPTime time) {
  char buffer[64];
  size_t ret = time.strftime(buffer, sizeof(buffer), format);
  if (ret > 0)
    return this->print(pos, buffer);
  return 0;
}
uint8_t Display::strftime(const char *format, ESPTime time) { return this->strftime(0, format, time); }

}  // namespace display_7segment_base
}  // namespace esphome
