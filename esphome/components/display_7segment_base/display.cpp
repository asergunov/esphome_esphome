#include "display.h"

#include "esphome/core/log.h"

namespace esphome {
namespace display_7segment_base {

static const char *const TAG = "display.display_7segment_base";

constexpr auto UNKNOWN_CHAR = Display::UNKNOWN_CHAR;

// starts from ' ' ord 0x20
constexpr uint8_t ASCII_TO_SEGMENTS[95] = {
    0b00000000,    // ' ', ord 0x20
    0b10110000,    // '!', ord 0x21
    0b00100010,    // '"', ord 0x22
    UNKNOWN_CHAR,  // '#', ord 0x23
    UNKNOWN_CHAR,  // '$', ord 0x24
    0b01001001,    // '%', ord 0x25
    UNKNOWN_CHAR,  // '&', ord 0x26
    0b00000010,    // ''', ord 0x27
    0b01001110,    // '(', ord 0x28
    0b01111000,    // ')', ord 0x29
    0b01000000,    // '*', ord 0x2A
    UNKNOWN_CHAR,  // '+', ord 0x2B
    0b00010000,    // ',', ord 0x2C
    0b00000001,    // '-', ord 0x2D
    0b10000000,    // '.', ord 0x2E
    UNKNOWN_CHAR,  // '/', ord 0x2F
    0b01111110,    // '0', ord 0x30
    0b00110000,    // '1', ord 0x31
    0b01101101,    // '2', ord 0x32
    0b01111001,    // '3', ord 0x33
    0b00110011,    // '4', ord 0x34
    0b01011011,    // '5', ord 0x35
    0b01011111,    // '6', ord 0x36
    0b01110000,    // '7', ord 0x37
    0b01111111,    // '8', ord 0x38
    0b01111011,    // '9', ord 0x39
    0b01001000,    // ':', ord 0x3A
    0b01011000,    // ';', ord 0x3B
    0b01000011,    // '<', ord 0x3C
    0b00001001,    // '=', ord 0x3D
    0b01100001,    // '>', ord 0x3E
    0b01100101,    // '?', ord 0x3F
    0b01101111,    // '@', ord 0x40
    0b01110111,    // 'A', ord 0x41
    0b01111111,    // 'B', ord 0x42
    0b01001110,    // 'C', ord 0x43
    0b01111001,    // 'D', ord 0x44
    0b01001111,    // 'E', ord 0x45
    0b01000111,    // 'F', ord 0x46
    0b01011110,    // 'G', ord 0x47
    0b00110111,    // 'H', ord 0x48
    0b00000110,    // 'I', ord 0x49
    0b00111000,    // 'J', ord 0x4A
    0b01010111,    // 'K', ord 0x4B
    0b00001110,    // 'L', ord 0x4C
    0b01101011,    // 'M', ord 0x4D
    0b01110110,    // 'N', ord 0x4E
    0b01111110,    // 'O', ord 0x4F
    0b01100111,    // 'P', ord 0x50
    0b01101011,    // 'Q', ord 0x51
    0b01101111,    // 'R', ord 0x52
    0b01011011,    // 'S', ord 0x53
    0b01000110,    // 'T', ord 0x54
    0b00111110,    // 'U', ord 0x55
    0b00111010,    // 'V', ord 0x56
    0b01011100,    // 'W', ord 0x57
    0b01001001,    // 'X', ord 0x58
    0b00101011,    // 'Y', ord 0x59
    0b01101101,    // 'Z', ord 0x5A
    0b01001110,    // '[', ord 0x5B
    UNKNOWN_CHAR,  // '\', ord 0x5C
    0b01111000,    // ']', ord 0x5D
    UNKNOWN_CHAR,  // '^', ord 0x5E
    0b00001000,    // '_', ord 0x5F
    0b00100000,    // '`', ord 0x60
    0b00011001,    // 'a', ord 0x61
    0b00011111,    // 'b', ord 0x62
    0b00001101,    // 'c', ord 0x63
    0b00111101,    // 'd', ord 0x64
    0b00001100,    // 'e', ord 0x65
    0b00000111,    // 'f', ord 0x66
    0b01001101,    // 'g', ord 0x67
    0b00010111,    // 'h', ord 0x68
    0b01001100,    // 'i', ord 0x69
    0b01011000,    // 'j', ord 0x6A
    0b01011000,    // 'k', ord 0x6B
    0b00000110,    // 'l', ord 0x6C
    0b01010101,    // 'm', ord 0x6D
    0b00010101,    // 'n', ord 0x6E
    0b00011101,    // 'o', ord 0x6F
    0b01100111,    // 'p', ord 0x70
    0b01110011,    // 'q', ord 0x71
    0b00000101,    // 'r', ord 0x72
    0b00011000,    // 's', ord 0x73
    0b00001111,    // 't', ord 0x74
    0b00011100,    // 'u', ord 0x75
    0b00011000,    // 'v', ord 0x76
    0b00101010,    // 'w', ord 0x77
    0b00001001,    // 'x', ord 0x78
    0b00111011,    // 'y', ord 0x79
    0b00001001,    // 'z', ord 0x7A
    0b00110001,    // '{', ord 0x7B
    0b00000110,    // '|', ord 0x7C
    0b00000111,    // '}', ord 0x7D
    0b01100011,    // '~', ord 0x7E (degree symbol)
};

/// Starts from 0x0410 to 0x044f
constexpr uint8_t CYRILLIC_TO_SEGMENTS[] = {
    0b01110111,    // `А` 0x0410
    0b01011111,    // `Б` 0x0411
    0b01111111,    // `В` 0x0412
    0b01000110,    // `Г` 0x0413
    0b01101010,    // `Д` 0x0414
    0b01001111,    // `Е` 0x0415
    UNKNOWN_CHAR,  // `Ж` 0x0416
    0b01111001,    // `З` 0x0417
    0b00111110,    // `И` 0x0418
    UNKNOWN_CHAR,  // `Й` 0x0419
    0b01010111,    // `К` 0x041a
    UNKNOWN_CHAR,  // `Л` 0x041b
    0b01101011,    // `М` 0x041c
    0b00110111,    // `Н` 0x041d
    0b01111110,    // `О` 0x041e
    0b01110110,    // `П` 0x041f
    0b01100111,    // `Р` 0x0420
    0b01001110,    // `С` 0x0421
    0b01110000,    // `Т` 0x0422
    0b00111011,    // `У` 0x0423
    UNKNOWN_CHAR,  // `Ф` 0x0424
    UNKNOWN_CHAR,  // `Х` 0x0425
    UNKNOWN_CHAR,  // `Ц` 0x0426
    0b00110011,    // `Ч` 0x0427
    UNKNOWN_CHAR,  // `Ш` 0x0428
    UNKNOWN_CHAR,  // `Щ` 0x0429
    UNKNOWN_CHAR,  // `Ъ` 0x042a
    UNKNOWN_CHAR,  // `Ы` 0x042b
    0b00011111,    // `Ь` 0x042c
    0b01111001,    // `Э` 0x042d
    UNKNOWN_CHAR,  // `Ю` 0x042e
    0b01110001,    // `Я` 0x042f

    0b00111101,    // `а` 0x0430
    0b00011111,    // `б` 0x0431
    UNKNOWN_CHAR,  // `в` 0x0432
    0b00000101,    // `г` 0x0433
    0b01111101,    // `д` 0x0434
    UNKNOWN_CHAR,  // `е` 0x0435
    UNKNOWN_CHAR,  // `ж` 0x0436
    UNKNOWN_CHAR,  // `з` 0x0437
    0b00011100,    // `и` 0x0438
    0b01011100,    // `й` 0x0439
    UNKNOWN_CHAR,  // `к` 0x043a
    UNKNOWN_CHAR,  // `л` 0x043b
    UNKNOWN_CHAR,  // `м` 0x043c
    UNKNOWN_CHAR,  // `н` 0x043d
    0b00011101,    // `о` 0x043e
    0b00010101,    // `п` 0x043f
    UNKNOWN_CHAR,  // `р` 0x0440
    0b00001101,    // `с` 0x0441
    0b00010001,    // `т` 0x0442
    UNKNOWN_CHAR,  // `у` 0x0443
    UNKNOWN_CHAR,  // `ф` 0x0444
    UNKNOWN_CHAR,  // `х` 0x0445
    UNKNOWN_CHAR,  // `ц` 0x0446
    UNKNOWN_CHAR,  // `ч` 0x0447
    UNKNOWN_CHAR,  // `ш` 0x0448
    UNKNOWN_CHAR,  // `щ` 0x0449
    UNKNOWN_CHAR,  // `ъ` 0x044a
    UNKNOWN_CHAR,  // `ы` 0x044b
    UNKNOWN_CHAR,  // `ь` 0x044c
    UNKNOWN_CHAR,  // `э` 0x044d
    UNKNOWN_CHAR,  // `ю` 0x044e
    UNKNOWN_CHAR,  // `я` 0x044f

    // UNKNOWN_CHAR, // `Ё` 0x0401
    // UNKNOWN_CHAR, // `ё` 0x0451
};

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

const char *Display::char_to_segments_(const char *str, uint8_t &segments) {
  uint32_t ucode = *(str++);
  // Find first octet
  while ((ucode & 0b11000000) == 0b10000000) {
    ESP_LOGD(TAG, "Skipping middle octet %x", ucode);
    ucode = *(str++);
  }

  ESP_LOGD(TAG, "First byte is %x", ucode);

  // Read U+ code
  uint8_t octets = 0;
  for (uint8_t bit = 0b10000000;; bit >>= 1) {
    ESP_LOGD(TAG, "Checking bit %x", bit);
    if (bit == 0) {
      ESP_LOGE(TAG, "utf-8: to many octets");
      return str;
    }

    if (0 == (bit & ucode)) {
      ESP_LOGD(TAG, "The bit is not set. Break.");
      break;
    }
    ++octets;
    // reset bit
    ucode &= ~bit;
    ESP_LOGD(TAG, "Unset the bit. Got %x", ucode);
  }

  ESP_LOGD(TAG, "Expected %d octets. Partial ucode %x", octets, ucode);

  for (; octets > 1; --octets) {
    uint8_t octet = *(str++);
    ESP_LOGD(TAG, "Parsing octet %x", octet);

    if ((octet & 0b11000000) != 0b10000000) {
      ESP_LOGE(TAG, "utf-8: bad octet %x", octet);
      return str;
    }

    octet &= 0b00111111;
    ESP_LOGD(TAG, "Meaningfull bits are %x", octet);

    ucode <<= uint32_t(6);
    ucode |= uint32_t(octet);
    ESP_LOGD(TAG, "Partial ucode %x", octets, ucode);
  }

  if (ucode == 0)
    return nullptr;

  if (ucode >= ' ' && ucode <= '~') {
    segments = progmem_read_byte(&ASCII_TO_SEGMENTS[ucode - ' ']);
  } else if (ucode >= 0x0410 && ucode < 0x0450) {
    segments = progmem_read_byte(&CYRILLIC_TO_SEGMENTS[ucode - 0x0410]);
  } else {
    segments = UNKNOWN_CHAR;
  }

  if (segments == UNKNOWN_CHAR) {
    ESP_LOGW(TAG, "Encountered character '%c' with no representation while translating string!", *str);
  }

  return str;
}

}  // namespace display_7segment_base
}  // namespace esphome
