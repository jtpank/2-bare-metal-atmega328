#ifndef SPI_HPP
#define SPI_HPP

#include <avr/io.h>
#include <stdint.h>

//This is in progress using the open source
// Arduino SPI library as a baseline
enum class SpiMode : uint8_t {
  SPI_MODE0 = 0,
  SPI_MODE1 = (1 << CPHA),
  SPI_MODE2 = (1 << CPOL),
  SPI_MODE3 = ((1 << CPOL) | (1 << CPHA))
};

class SPI
{
  public:
    void spi_init();
    void spi_set_mode(SpiMode mode);
    uint8_t spi_transfer(uint8_t data);
    void cs_low();
    void cs_high();

  private:
    SpiMode m_mode{SpiMode::SPI_MODE0};

};
#endif
