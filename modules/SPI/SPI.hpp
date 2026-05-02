#ifndef SPI_HPP
#define SPI_HPP

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stddef.h>

#ifndef F_CPU
#error "F_CPU must be defined, e.g. -DF_CPU=16000000UL"
#endif

enum class SpiBitOrder : uint8_t {
  LSB_FIRST = 0,
  MSB_FIRST = 1
};

enum class SpiMode : uint8_t {
  MODE0 = 0,
  MODE1 = _BV(CPHA),
  MODE2 = _BV(CPOL),
  MODE3 = _BV(CPOL) | _BV(CPHA)
};

enum class SpiClockDiv : uint8_t {
  CLOCK_DIV4   = 0x00,
  CLOCK_DIV16  = 0x01,
  CLOCK_DIV64  = 0x02,
  CLOCK_DIV128 = 0x03,
  CLOCK_DIV2   = 0x04,
  CLOCK_DIV8   = 0x05,
  CLOCK_DIV32  = 0x06
};

namespace SpiMask {
  static constexpr uint8_t MODE    = _BV(CPOL) | _BV(CPHA); // 0x0C
  static constexpr uint8_t CLOCK   = _BV(SPR1) | _BV(SPR0); // 0x03
  static constexpr uint8_t CLOCK2X = _BV(SPI2X);            // 0x01
}

class SPISettings {
public:
  SPISettings(
    uint32_t clock,
    SpiBitOrder bitOrder = SpiBitOrder::MSB_FIRST,
    SpiMode dataMode = SpiMode::MODE0
  ) {
    init(clock, bitOrder, dataMode);
  }

  SPISettings() {
    init(4000000UL, SpiBitOrder::MSB_FIRST, SpiMode::MODE0);
  }

private:
  uint8_t spcr{};
  uint8_t spsr{};

  void init(uint32_t clock, SpiBitOrder bitOrder, SpiMode dataMode) {
    uint8_t clockDiv;

    if (__builtin_constant_p(clock)) {
      if (clock >= F_CPU / 2) {
        clockDiv = 0;
      } else if (clock >= F_CPU / 4) {
        clockDiv = 1;
      } else if (clock >= F_CPU / 8) {
        clockDiv = 2;
      } else if (clock >= F_CPU / 16) {
        clockDiv = 3;
      } else if (clock >= F_CPU / 32) {
        clockDiv = 4;
      } else if (clock >= F_CPU / 64) {
        clockDiv = 5;
      } else {
        clockDiv = 6;
      }
    } else {
      uint32_t clockSetting = F_CPU / 2;
      clockDiv = 0;

      while (clockDiv < 6 && clock < clockSetting) {
        clockSetting /= 2;
        clockDiv++;
      }
    }

    // Arduino-compatible encoding:
    // clockDiv 0..6 maps to fosc/2..fosc/128, but fosc/64 appears twice.
    if (clockDiv == 6) {
      clockDiv = 7;
    }

    // Invert SPI2X bit.
    clockDiv ^= 0x01;

    spcr =
      _BV(SPE) |
      _BV(MSTR) |
      (bitOrder == SpiBitOrder::LSB_FIRST ? _BV(DORD) : 0) |
      static_cast<uint8_t>(dataMode) |
      ((clockDiv >> 1) & SpiMask::CLOCK);

    spsr = clockDiv & SpiMask::CLOCK2X;
  }

  friend class SPIClass;
};

class SPIClass {
public:
  static void begin() {
    // ATmega328P hardware SPI pins:
    // PB0 = D8  / RF24 CSN
    // PB1 = D9  / RF24 CE
    // PB2 = D10 / SS, must stay OUTPUT for SPI master mode
    // PB3 = MOSI
    // PB4 = MISO
    // PB5 = SCK

    DDRB |= _BV(DDB0);   // D8 / RF24 CSN output
    DDRB |= _BV(DDB1);   // D9 / RF24 CE output

    DDRB |= _BV(DDB2); // SS output
    DDRB |= _BV(DDB3); // MOSI output
    DDRB &= ~_BV(DDB4); // MISO input
    DDRB |= _BV(DDB5); // SCK output

    // Keep SS high so the AVR stays SPI master.
    PORTB |= _BV(PORTB0); // CSN high / deselect RF24
    PORTB &= ~_BV(PORTB1); // CE low

    PORTB |= _BV(PORTB2);

    SPCR = _BV(SPE) | _BV(MSTR);
    SPSR = 0;
  }

  static void end() {
    SPCR &= ~_BV(SPE);
  }

  static void beginTransaction(const SPISettings& settings) {
    uint8_t sreg = SREG;
    cli();

    SPCR = settings.spcr;
    SPSR = settings.spsr;

    SREG = sreg;
  }

  static void endTransaction() {
    // Nothing required for simple bare-metal polling SPI.
    // Kept for API symmetry.
  }

  static uint8_t transfer(uint8_t data) {
    SPDR = data;

    asm volatile("nop");

    while (!(SPSR & _BV(SPIF))) {
      // wait
    }

    return SPDR;
  }

  static uint16_t transfer16(uint16_t data) {
    union {
      uint16_t val;
      struct {
        uint8_t lsb;
        uint8_t msb;
      };
    } in, out;

    in.val = data;

    if (!(SPCR & _BV(DORD))) {
      SPDR = in.msb;
      asm volatile("nop");
      while (!(SPSR & _BV(SPIF))) {}
      out.msb = SPDR;

      SPDR = in.lsb;
      asm volatile("nop");
      while (!(SPSR & _BV(SPIF))) {}
      out.lsb = SPDR;
    } else {
      SPDR = in.lsb;
      asm volatile("nop");
      while (!(SPSR & _BV(SPIF))) {}
      out.lsb = SPDR;

      SPDR = in.msb;
      asm volatile("nop");
      while (!(SPSR & _BV(SPIF))) {}
      out.msb = SPDR;
    }

    return out.val;
  }

  static void transfer(void* buffer, size_t count) {
    if (count == 0) {
      return;
    }

    uint8_t* p = static_cast<uint8_t*>(buffer);

    SPDR = *p;

    while (--count > 0) {
      uint8_t out = *(p + 1);

      while (!(SPSR & _BV(SPIF))) {}

      uint8_t in = SPDR;
      SPDR = out;
      *p++ = in;
    }

    while (!(SPSR & _BV(SPIF))) {}

    *p = SPDR;
  }

  static void setBitOrder(SpiBitOrder bitOrder) {
    if (bitOrder == SpiBitOrder::LSB_FIRST) {
      SPCR |= _BV(DORD);
    } else {
      SPCR &= ~_BV(DORD);
    }
  }

  static void setDataMode(SpiMode mode) {
    SPCR = (SPCR & ~SpiMask::MODE) | static_cast<uint8_t>(mode);
  }

  static void setClockDivider(SpiClockDiv clockDiv) {
    uint8_t div = static_cast<uint8_t>(clockDiv);

    SPCR = (SPCR & ~SpiMask::CLOCK) | (div & SpiMask::CLOCK);
    SPSR = (SPSR & ~SpiMask::CLOCK2X) | ((div >> 2) & SpiMask::CLOCK2X);
  }

  static void attachInterrupt() {
    SPCR |= _BV(SPIE);
  }

  static void detachInterrupt() {
    SPCR &= ~_BV(SPIE);
  }

  static void csLow() {
    PORTB &= ~_BV(PORTB0);
  }

  static void csHigh() {
    PORTB |= _BV(PORTB0);
  }
};

extern SPIClass SPI;

#endif
