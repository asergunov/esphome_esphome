#pragma once

#include <cstdint>

namespace vs1xxx_registers {

enum class Access { R, W, RW };

template<typename _REG, uint8_t _LAST_BIT = 7, uint8_t _FIRST_BIT = 0> struct Field;

template<uint8_t _REG_ADR, Access _ACCESS = Access::RW, uint16_t _POR_STATE = 0x00> struct Register {
  static constexpr auto REG_ADR = _REG_ADR;
  static constexpr auto POR_STATE = _POR_STATE;

  explicit constexpr Register(uint16_t val) : value(val) {}
  constexpr Register() {}

  template<uint8_t _LAST_BIT, uint8_t _FIRST_BIT>
  Register &operator<<(const Field<Register, _LAST_BIT, _FIRST_BIT> &field) {
    using Field = Field<Register, _LAST_BIT, _FIRST_BIT>;
    value = (value & ~Field::MASK) | (Field::MASK & (static_cast<uint16_t>(field) << Field::BEGIN_BIT));
    return *this;
  }

  template<uint8_t _LAST_BIT, uint8_t _FIRST_BIT>
  const Register &operator>>(Field<Register, _LAST_BIT, _FIRST_BIT> &field) const {
    using Field = Field<Register, _LAST_BIT, _FIRST_BIT>;
    field = Field((value & Field::MASK) >> Field::BEGIN_BIT);
    return *this;
  }

  constexpr operator const uint16_t &() const { return value; }
  operator uint16_t &() { return value; }

  Register operator=(const uint16_t &raw) {
    value = raw;
    return *this;
  }

 private:
  uint16_t value = POR_STATE;
};

template<typename _REG, uint8_t _LAST_BIT, uint8_t _FIRST_BIT> struct Field {
  static constexpr auto BEGIN_BIT = _FIRST_BIT;
  static constexpr auto END_BIT = _LAST_BIT + 1;
  static constexpr auto MASK = ((1u << (END_BIT - BEGIN_BIT + 1u)) - 1u) << BEGIN_BIT;
  using REG = _REG;

  Field() {}
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
  uint32_t clki_for_xtali(uint32_t xtali) const { return xtali * (2 + static_cast<uint16_t>(*this)) / 2; }
};
using SC_ADD = Field<SCI_CLOCKF, 12, 11>;
struct SC_FREQ : Field<SCI_CLOCKF, 10, 0> {
  using Field<SCI_CLOCKF, 10, 0>::Field;
  static SC_FREQ from_hz(uint32_t hz) { return SC_FREQ(static_cast<uint16_t>((hz - 8000000) / 4000)); }
};

}  // namespace vs1xxx_registers
