#ifndef RF24_HPP
#define RF24_HPP

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "SPI.hpp"
#include "nRF24L01.h"

#ifndef RF24_SPI_SPEED
#define RF24_SPI_SPEED 10000000UL
#endif

struct Rf24Pin {
  volatile uint8_t* ddr;
  volatile uint8_t* port;
  uint8_t bit;
};

#define RF24_PIN_B(n) Rf24Pin{&DDRB, &PORTB, static_cast<uint8_t>(n)}
#define RF24_PIN_C(n) Rf24Pin{&DDRC, &PORTC, static_cast<uint8_t>(n)}
#define RF24_PIN_D(n) Rf24Pin{&DDRD, &PORTD, static_cast<uint8_t>(n)}

enum rf24_pa_dbm_e : uint8_t {
  RF24_PA_MIN = 0,
  RF24_PA_LOW,
  RF24_PA_HIGH,
  RF24_PA_MAX,
  RF24_PA_ERROR
};

class RF24 {
public:
  RF24(Rf24Pin cePin, Rf24Pin csnPin, uint32_t spiSpeed = RF24_SPI_SPEED);

  bool begin();

  void setPALevel(rf24_pa_dbm_e level);
  void setPayloadSize(uint8_t size);
  uint8_t getPayloadSize() const;

  void openReadingPipe(uint8_t pipe, const uint8_t* address);

  void stopListening();
  void stopListening(const uint8_t* txAddress);
  void startListening();

  bool write(const void* buf, uint8_t len);

  bool available();
  bool available(uint8_t* pipeNum);

  void read(void* buf, uint8_t len);

private:
  Rf24Pin ce_pin;
  Rf24Pin csn_pin;
  uint32_t spi_speed;

  uint8_t payload_size = 32;
  uint8_t addr_width = 5;
  uint8_t status = 0;
  uint8_t pipe0_reading_address[5]{};

  static void pinModeOutput(const Rf24Pin& pin);
  static void pinHigh(const Rf24Pin& pin);
  static void pinLow(const Rf24Pin& pin);

  void ce(bool level);
  void csn(bool level);

  void beginTransaction();
  void endTransaction();

  uint8_t spiTransfer(uint8_t data);

  uint8_t readRegister(uint8_t reg);
  void readRegister(uint8_t reg, uint8_t* buf, uint8_t len);

  uint8_t writeRegister(uint8_t reg, uint8_t value);
  uint8_t writeRegister(uint8_t reg, const uint8_t* buf, uint8_t len);

  uint8_t writeCommand(uint8_t command);
  uint8_t flushRx();
  uint8_t flushTx();

  uint8_t getStatus();
};

#endif
