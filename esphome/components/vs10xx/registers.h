#pragma once

#include <cstdint>

namespace vs1xxx_registers {

enum class Access { R, W, RW };

template<typename _REG, uint8_t _LAST_BIT = 7, uint8_t _FIRST_BIT = 0> struct Field;

template<uint8_t _REG_ADR, Access _ACCESS = Access::RW, uint16_t _POR_STATE = 0x00> struct Register {
  static constexpr auto REG_ADR = _REG_ADR;
  static constexpr auto POR_STATE = _POR_STATE;

  explicit constexpr Register(uint16_t val) : _value(val) {}
  constexpr Register() {}

  bool operator==(const Register &rhs) { return _value == rhs._value; }
  bool operator!=(const Register &rhs) { return _value != rhs._value; }

  template<uint8_t _LAST_BIT, uint8_t _FIRST_BIT>
  Register &operator<<(const Field<Register, _LAST_BIT, _FIRST_BIT> &field) {
    using Field = Field<Register, _LAST_BIT, _FIRST_BIT>;
    _value = (_value & ~Field::MASK) | (Field::MASK & (field << Field::BEGIN_BIT));
    return *this;
  }

  template<typename Field> const Register &operator>>(Field &field) const {
    // using Field = Field<Register, _LAST_BIT, _FIRST_BIT>;
    field = Field((_value & Field::MASK) >> Field::BEGIN_BIT);
    return *this;
  }

  template<typename FIELD> FIELD read_field() const {
    FIELD ret;
    (*this) >> ret;
    return ret;
  }

  // constexpr operator const uint16_t &() const { return value; }
  // operator uint16_t &() { return value; }

  const uint16_t &value() const { return _value; }

  Register operator=(const uint16_t &raw) {
    _value = raw;
    return *this;
  }

 private:
  uint16_t _value = POR_STATE;
};

template<typename _REG, uint8_t _LAST_BIT, uint8_t _FIRST_BIT> struct Field {
  static constexpr auto BEGIN_BIT = _FIRST_BIT;
  static constexpr auto END_BIT = _LAST_BIT + 1;
  static constexpr auto MASK = uint16_t(((uint32_t(1) << (END_BIT - BEGIN_BIT)) - uint32_t(1)) << BEGIN_BIT);
  using REG = _REG;

  Field() {}
  constexpr Field(const Field &) = default;
  constexpr Field(const uint16_t &value) : _value(value) {}
  // constexpr Field(const REG &reg) : Field(static_cast<Field>(reg)) {}
  constexpr operator const uint16_t &() const { return _value; }

 private:
  uint16_t _value;
};

template<typename _REG, uint8_t _BIT> using Bit = Field<_REG, _BIT, _BIT>;

using SCI_MODE = Register<0x0>;
using SM_DIFF = Bit<SCI_MODE, 0>;
using SM_SETTOZERO = Bit<SCI_MODE, 1>;
using SM_RESET = Bit<SCI_MODE, 2>;
using SM_OUTOFWAV = Bit<SCI_MODE, 3>;
using SM_PDOWN = Bit<SCI_MODE, 4>;
using SM_TESTS = Bit<SCI_MODE, 5>;
using SM_STREAM = Bit<SCI_MODE, 6>;
using SM_SETTOZERO2 = Bit<SCI_MODE, 7>;
using SM_DACT = Bit<SCI_MODE, 8>;
using SM_SDIORD = Bit<SCI_MODE, 9>;
using SM_SDISHARE = Bit<SCI_MODE, 10>;
using SM_SDINEW = Bit<SCI_MODE, 11>;
using SM_ADPCM = Bit<SCI_MODE, 12>;
using SM_ADPCM_HP = Bit<SCI_MODE, 13>;
using SM_LINE_IN = Bit<SCI_MODE, 14>;

using SCI_STATUS = Register<0x1>;
struct SS_VER : Field<SCI_STATUS, 6, 4> {
  using Field<SCI_STATUS, 6, 4>::Field;
  const char *chip_name() const {
    switch (static_cast<uint16_t>(*this)) {
      case 0:
        return "VS1001";
      case 1:
        return "VS1011";
      case 2:
        return "VS1002";
      case 3:
        return "VS1003";
    }
    return "Unknown";
  }
};
using SS_APDOWN2 = Bit<SCI_STATUS, 3>;
using SS_APDOWN1 = Bit<SCI_STATUS, 2>;
using SS_AVOL = Field<SCI_STATUS, 1, 0>;

using SCI_BASS = Register<0x2>;
using ST_AMPLITUDE = Field<SCI_BASS, 15, 12>;
using ST_FREQLIMIT = Field<SCI_BASS, 11, 8>;
using SB_AMPLITUDE = Field<SCI_BASS, 7, 4>;
using SB_FREQLIMIT = Field<SCI_BASS, 3, 0>;

using SCI_CLOCKF = Register<0x3>;
struct SC_MULT : Field<SCI_CLOCKF, 15, 13> {
  using Field<SCI_CLOCKF, 15, 13>::Field;
  uint32_t clki_for_xtali(uint32_t xtali) const { return uint64_t(xtali) * (2 + static_cast<uint16_t>(*this)) / 2; }
};
using SC_ADD = Field<SCI_CLOCKF, 12, 11>;
struct SC_FREQ : Field<SCI_CLOCKF, 10, 0> {
  using Field<SCI_CLOCKF, 10, 0>::Field;
  static SC_FREQ from_hz(uint32_t hz) { return SC_FREQ(static_cast<uint16_t>((hz - 8000000) / 4000)); }
};

using SCI_DECODE_TIME = Register<0x4>;

struct SCI_AUDATA : Register<0x5> {
  using Register<0x5>::Register;
  using SC_CHANNELS = Bit<SCI_AUDATA, 0>;
  using SC_HALF_SAMPLERATE = Field<SCI_AUDATA, 15, 1>;

  bool is_stereo() const { return read_field<SC_CHANNELS>(); }
  uint16_t sample_rate() const { return read_field<SC_HALF_SAMPLERATE>() * 2; }
};

using SCI_WRAM = Register<0x6>;
using SCI_WRAMADDR = Register<0x7>;

enum class StreamType { None, Wav, Wma, Midi, Mp3, Unknown };

struct SCI_HDAT1 : Register<0x9, Access::R> {
  using MP3_SYNC_WORD = Field<SCI_HDAT1, 15, 5>;
  struct MP3_ID : Field<SCI_HDAT1, 4, 3> {
    using Field<SCI_HDAT1, 4, 3>::Field;
    const char *name() const {
      switch (*this) {
        case 3:
          return "ISO 11172-3 MPG 1.0";
        case 2:
          return "ISO 13818-3 MPG 2.0 (1/2-rate)";
        case 1:
        case 0:
          return "MPG 2.5 (1/4-rate)";
      }
      return "Unknown";
    }
  };
  struct MP3_LAYER : Field<SCI_HDAT1, 2, 1> {
    using Field<SCI_HDAT1, 2, 1>::Field;
    const char *name() const {
      switch (*this) {
        case 3:
          return "I";
        case 2:
          return "II";
        case 1:
          return "III";
      }
      return "Unknown";
    }
  };
  using MP3_PROTECT_BIT = Bit<SCI_HDAT1, 0>;

  using Register<0x9, Access::R>::Register;
  StreamType stream_type() const {
    switch (value()) {
      case 0x0000:
        return StreamType::None;
      case 0x7665:
        return StreamType::Wav;
      case 0x574D:
        return StreamType::Wma;
      case 0x4D54:
        return StreamType::Midi;
      default: {
        if (read_field<MP3_SYNC_WORD>() == 2047) {
          return StreamType::Mp3;
        }
      }
        return StreamType::Unknown;
    }
  }
};

struct SCI_HDAT0 : Register<0x8, Access::R> {
  using MIDI_TYPE = Field<SCI_HDAT0, 15, 5>;
  using MIDI_TYPE0_PLIPHONY = Field<SCI_HDAT0, 7, 0>;

  using MP3_BITRATE = Field<SCI_HDAT0, 15, 12>;
  using MP3_SAMPLE_RATE = Field<SCI_HDAT0, 11, 10>;
  using MP3_PAD_BIT = Bit<SCI_HDAT0, 9>;
  using MP3_PRIVATE_BIT = Bit<SCI_HDAT0, 9>;
  struct MP3_MODE : Field<SCI_HDAT0, 7, 6> {
    using Field<SCI_HDAT0, 7, 6>::Field;
    const char *name() const {
      switch (*this) {
        case 3:
          return "mono";
        case 2:
          return "dual channel";
        case 1:
          return "joint stereo";
        case 0:
          return "stereo";
      }
      return "Unknown";
    }
  };
  using MP3_EXTENSION = Field<SCI_HDAT0, 5, 4>;
  using MP3_COPYRIGHT = Bit<SCI_HDAT0, 3>;
  using MP3_ORIGINAL = Bit<SCI_HDAT0, 2>;
  struct MP3_EMPHASIS : Field<SCI_HDAT0, 1, 0> {
    using Field<SCI_HDAT0, 1, 0>::Field;
    const char *name() const {
      switch (*this) {
        case 3:
          return "CCITT J.17";
        case 2:
          return "reserved";
        case 1:
          return "50/15 microsec";
        case 0:
          return "none";
      }
      return "Unknown";
    }
  };

  using Register<0x8, Access::R>::Register;

  uint16_t wma_bitrate() const { return value() * 8 / 1025; }

  uint16_t mp3_bitrate(const SCI_HDAT1::MP3_ID &id) const {
    switch (id) {
      case 3: {
        const uint16_t rates[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
        return rates[read_field<MP3_BITRATE>()];
      }
      case 0:
      case 1:
      case 2: {
        const uint16_t rates[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
        return rates[read_field<MP3_BITRATE>()];
      }
    }
    return 0;
  }

  uint16_t mp3_sample_rate(const SCI_HDAT1::MP3_ID &id) const {
    switch (id) {
      case 3: {
        const uint16_t rates[] = {44100, 48000, 32000, 0};
        return rates[read_field<MP3_SAMPLE_RATE>()];
      }
      case 2: {
        const uint16_t rates[] = {22050, 24000, 16000, 0};
        return rates[read_field<MP3_SAMPLE_RATE>()];
      }
      case 0:
      case 1: {
        const uint16_t rates[] = {11025, 12000, 8000, 0};
        return rates[read_field<MP3_SAMPLE_RATE>()];
      }
    }
    return 0;
  }
};

using SCI_AIADDR = Register<0xa>;
using SCI_VOL = Register<0xb>;

using SCI_AICTRL0 = Register<0xc>;
using SCI_AICTRL1 = Register<0xc>;
using SCI_AICTRL2 = Register<0xc>;
using SCI_AICTRL3 = Register<0xc>;

}  // namespace vs1xxx_registers
