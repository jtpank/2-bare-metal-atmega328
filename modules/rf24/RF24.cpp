#include "RF24.hpp"

RF24::RF24(Rf24Pin cePin, Rf24Pin csnPin, uint32_t spiSpeed)
  : ce_pin(cePin),
    csn_pin(csnPin),
    spi_speed(spiSpeed)
{
}

void RF24::pinModeOutput(const Rf24Pin& pin) {
  *pin.ddr |= _BV(pin.bit);
}

void RF24::pinHigh(const Rf24Pin& pin) {
  *pin.port |= _BV(pin.bit);
}

void RF24::pinLow(const Rf24Pin& pin) {
  *pin.port &= ~_BV(pin.bit);
}

void RF24::ce(bool level) {
  if (level) {
    pinHigh(ce_pin);
  } else {
    pinLow(ce_pin);
  }
}

void RF24::csn(bool level) {
  if (level) {
    pinHigh(csn_pin);
  } else {
    pinLow(csn_pin);
  }
}

void RF24::beginTransaction() {
  SPI.beginTransaction(
    SPISettings(
      spi_speed,
      SpiBitOrder::MSB_FIRST,
      SpiMode::MODE0
    )
  );

  csn(false);
}

void RF24::endTransaction() {
  csn(true);
  SPI.endTransaction();
}

uint8_t RF24::spiTransfer(uint8_t data) {
  return SPI.transfer(data);
}

uint8_t RF24::readRegister(uint8_t reg) {
  beginTransaction();

  status = spiTransfer(nRF24L01::R_REGISTER | (nRF24L01::REGISTER_MASK & reg));
  uint8_t result = spiTransfer(0xFF);

  endTransaction();

  return result;
}

void RF24::readRegister(uint8_t reg, uint8_t* buf, uint8_t len) {
  beginTransaction();

  status = spiTransfer(nRF24L01::R_REGISTER | (nRF24L01::REGISTER_MASK & reg));

  while (len--) {
    *buf++ = spiTransfer(0xFF);
  }

  endTransaction();
}

uint8_t RF24::writeRegister(uint8_t reg, uint8_t value) {
  beginTransaction();

  status = spiTransfer(nRF24L01::W_REGISTER | (nRF24L01::REGISTER_MASK & reg));
  spiTransfer(value);

  endTransaction();

  return status;
}

uint8_t RF24::writeRegister(uint8_t reg, const uint8_t* buf, uint8_t len) {
  beginTransaction();

  status = spiTransfer(nRF24L01::W_REGISTER | (nRF24L01::REGISTER_MASK & reg));

  while (len--) {
    spiTransfer(*buf++);
  }

  endTransaction();

  return status;
}

uint8_t RF24::writeCommand(uint8_t command) {
  beginTransaction();

  status = spiTransfer(command);

  endTransaction();

  return status;
}

uint8_t RF24::flushRx() {
  return writeCommand(nRF24L01::FLUSH_RX);
}

uint8_t RF24::flushTx() {
  return writeCommand(nRF24L01::FLUSH_TX);
}

uint8_t RF24::getStatus() {
  beginTransaction();

  status = spiTransfer(nRF24L01::NOP);

  endTransaction();

  return status;
}

bool RF24::begin() {
  pinModeOutput(ce_pin);
  pinModeOutput(csn_pin);

  ce(false);
  csn(true);

  SPI.begin();

  _delay_ms(5);

  // 5-byte addresses
  addr_width = 5;
  writeRegister(nRF24L01::SETUP_AW, 0x03);

  // 1500 us retry delay, 15 retries
  writeRegister(nRF24L01::SETUP_RETR, 0x5F);

  // Default channel 76
  writeRegister(nRF24L01::RF_CH, 76);

  // 1 Mbps, max PA by default
  writeRegister(
    nRF24L01::RF_SETUP,
    _BV(nRF24L01::RF_PWR_LOW) | _BV(nRF24L01::RF_PWR_HIGH)
  );

  // Enable auto-ack on pipes 0 and 1
  writeRegister(nRF24L01::EN_AA, _BV(nRF24L01::ENAA_P0) | _BV(nRF24L01::ENAA_P1));

  // Enable RX addresses pipe 0 and 1
  writeRegister(nRF24L01::EN_RXADDR, _BV(nRF24L01::ERX_P0) | _BV(nRF24L01::ERX_P1));

  // Default fixed payload size
  setPayloadSize(32);

  // Clear IRQ flags
  writeRegister(
    nRF24L01::STATUS,
    _BV(nRF24L01::RX_DR) | _BV(nRF24L01::TX_DS) | _BV(nRF24L01::MAX_RT)
  );

  flushRx();
  flushTx();

  // Power up, CRC enabled, TX mode initially
  writeRegister(nRF24L01::CONFIG, _BV(nRF24L01::EN_CRC) | _BV(nRF24L01::PWR_UP));

  _delay_ms(5);

  // Basic sanity check
  uint8_t setupAw = readRegister(nRF24L01::SETUP_AW);
  return setupAw >= 1 && setupAw <= 3;
}

void RF24::setPALevel(rf24_pa_dbm_e level) {
  uint8_t setup = readRegister(nRF24L01::RF_SETUP);

  setup &= ~(_BV(nRF24L01::RF_PWR_LOW) | _BV(nRF24L01::RF_PWR_HIGH));

  switch (level) {
    case RF24_PA_MIN:
      break;

    case RF24_PA_LOW:
      setup |= _BV(nRF24L01::RF_PWR_LOW);
      break;

    case RF24_PA_HIGH:
      setup |= _BV(nRF24L01::RF_PWR_HIGH);
      break;

    case RF24_PA_MAX:
    default:
      setup |= _BV(nRF24L01::RF_PWR_LOW) | _BV(nRF24L01::RF_PWR_HIGH);
      break;
  }

  writeRegister(nRF24L01::RF_SETUP, setup);
}

void RF24::setPayloadSize(uint8_t size) {
  if (size == 0) {
    size = 1;
  }

  if (size > 32) {
    size = 32;
  }

  payload_size = size;

  writeRegister(nRF24L01::RX_PW_P0, payload_size);
  writeRegister(nRF24L01::RX_PW_P1, payload_size);
}

uint8_t RF24::getPayloadSize() const {
  return payload_size;
}

void RF24::openReadingPipe(uint8_t pipe, const uint8_t* address) {
  if (pipe > 5) {
    return;
  }

  if (pipe == 0) {
    memcpy(pipe0_reading_address, address, addr_width);
    writeRegister(nRF24L01::RX_ADDR_P0, address, addr_width);
    writeRegister(nRF24L01::RX_PW_P0, payload_size);
  } else if (pipe == 1) {
    writeRegister(nRF24L01::RX_ADDR_P1, address, addr_width);
    writeRegister(nRF24L01::RX_PW_P1, payload_size);
  } else {
    // Pipes 2-5 only take the least-significant byte.
    writeRegister(nRF24L01::RX_ADDR_P0 + pipe, address[0]);
    writeRegister(nRF24L01::RX_PW_P0 + pipe, payload_size);
  }

  uint8_t enRxAddr = readRegister(nRF24L01::EN_RXADDR);
  enRxAddr |= _BV(pipe);
  writeRegister(nRF24L01::EN_RXADDR, enRxAddr);
}

void RF24::stopListening() {
  ce(false);

  _delay_us(150);

  uint8_t config = readRegister(nRF24L01::CONFIG);
  config &= ~_BV(nRF24L01::PRIM_RX);
  writeRegister(nRF24L01::CONFIG, config);

  _delay_us(150);

  flushTx();

  // Restore pipe 0 payload size for TX auto-ack behavior.
  writeRegister(nRF24L01::RX_PW_P0, payload_size);
}

void RF24::stopListening(const uint8_t* txAddress) {
  stopListening();

  // TX address
  writeRegister(nRF24L01::TX_ADDR, txAddress, addr_width);

  // Pipe 0 must mirror TX address for auto-ack.
  writeRegister(nRF24L01::RX_ADDR_P0, txAddress, addr_width);
}

void RF24::startListening() {
  uint8_t config = readRegister(nRF24L01::CONFIG);
  config |= _BV(nRF24L01::PRIM_RX);
  config |= _BV(nRF24L01::PWR_UP);
  writeRegister(nRF24L01::CONFIG, config);

  writeRegister(
    nRF24L01::STATUS,
    _BV(nRF24L01::RX_DR) | _BV(nRF24L01::TX_DS) | _BV(nRF24L01::MAX_RT)
  );

  // Restore original pipe 0 reading address if one was set.
  writeRegister(nRF24L01::RX_ADDR_P0, pipe0_reading_address, addr_width);

  flushRx();
  flushTx();

  ce(true);

  _delay_us(150);
}

bool RF24::write(const void* buf, uint8_t len) {
  if (len > 32) {
    len = 32;
  }

  stopListening();

  writeRegister(
    nRF24L01::STATUS,
    _BV(nRF24L01::RX_DR) | _BV(nRF24L01::TX_DS) | _BV(nRF24L01::MAX_RT)
  );

  flushTx();

  beginTransaction();

  status = spiTransfer(nRF24L01::W_TX_PAYLOAD);

  const uint8_t* current = static_cast<const uint8_t*>(buf);

  for (uint8_t i = 0; i < len; ++i) {
    spiTransfer(current[i]);
  }

  // Pad to fixed payload size.
  for (uint8_t i = len; i < payload_size; ++i) {
    spiTransfer(0);
  }

  endTransaction();

  ce(true);
  _delay_us(15);
  ce(false);

  // Wait for TX success or max retries.
  uint16_t timeout = 60000;

  while (timeout--) {
    uint8_t s = getStatus();

    if (s & _BV(nRF24L01::TX_DS)) {
      writeRegister(nRF24L01::STATUS, _BV(nRF24L01::TX_DS));
      return true;
    }

    if (s & _BV(nRF24L01::MAX_RT)) {
      writeRegister(nRF24L01::STATUS, _BV(nRF24L01::MAX_RT));
      flushTx();
      return false;
    }
  }

  flushTx();
  return false;
}

bool RF24::available() {
  return available(nullptr);
}

bool RF24::available(uint8_t* pipeNum) {
  uint8_t s = getStatus();

  uint8_t pipe = (s >> nRF24L01::RX_P_NO) & 0x07;

  if (pipeNum) {
    *pipeNum = pipe;
  }

  if (pipe > 5) {
    return false;
  }

  uint8_t fifo = readRegister(nRF24L01::FIFO_STATUS);

  return !(fifo & _BV(nRF24L01::RX_EMPTY));
}

void RF24::read(void* buf, uint8_t len) {
  if (len > 32) {
    len = 32;
  }

  beginTransaction();

  status = spiTransfer(nRF24L01::R_RX_PAYLOAD);

  uint8_t* current = static_cast<uint8_t*>(buf);

  for (uint8_t i = 0; i < len; ++i) {
    current[i] = spiTransfer(0xFF);
  }

  // Drain remainder of fixed payload if caller reads fewer bytes.
  for (uint8_t i = len; i < payload_size; ++i) {
    spiTransfer(0xFF);
  }

  endTransaction();

  writeRegister(nRF24L01::STATUS, _BV(nRF24L01::RX_DR));
}
