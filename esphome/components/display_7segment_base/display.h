#pragma once

#include "esphome/core/component.h"
#include "esphome/core/time.h"
#include "esphome/core/hal.h"

#include <cstdarg>

namespace esphome {
namespace display_7segment_base {

class Display : public PollingComponent {
 public:
  static constexpr uint8_t UNKNOWN_CHAR = 0xff;

  /// Evaluate the printf-format and print the result at the given position.
  uint8_t printf(uint8_t pos, const char *format, ...) __attribute__((format(printf, 3, 4)));
  /// Evaluate the printf-format and print the result at position 0.
  uint8_t printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

  /// Print `str` at the given position.
  uint8_t print(uint8_t pos, const char *str) { return print_(pos, str); }

  /// Print `str` at position 0.
  uint8_t print(const char *str);

  /// Evaluate the strftime-format and print the result at the given position.
  uint8_t strftime(uint8_t pos, const char *format, ESPTime time) __attribute__((format(strftime, 3, 0)));

  /// Evaluate the strftime-format and print the result at position 0.
  uint8_t strftime(const char *format, ESPTime time) __attribute__((format(strftime, 2, 0)));

  /// Convert utf-8 char to 7 segments. 0bXABCDEFG
  ///
  ///      A
  ///     ---
  ///  F |   | B
  ///     -G-
  ///  E |   | C
  ///     ---
  ///      D   X
  static const char *char_to_segments_(const char *str, uint8_t &segments);

 protected:
  virtual uint8_t print_(uint8_t pos, const char *str) = 0;
};

}  // namespace display_7segment_base
}  // namespace esphome
