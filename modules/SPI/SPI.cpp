#include "SPI.hpp"

void SPI::spi_init()
{
    // ATmega328P SPI pins:
    // PB2 = SS   / Arduino D10
    // PB3 = MOSI / Arduino D11
    // PB4 = MISO / Arduino D12
    // PB5 = SCK  / Arduino D13

    DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB5); // SS, MOSI, SCK output
    DDRB &= ~(1 << PB4);                          // MISO input

    // Keep SS high so this AVR stays SPI master
    PORTB |= (1 << PB2);

    // Enable SPI, master mode, clock = F_CPU / 16
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

    // Disable double-speed SPI
    SPSR &= ~(1 << SPI2X);

    spi_set_mode(m_mode);
}

void SPI::spi_set_mode(SpiMode mode)
{
    m_mode = mode;

    uint8_t mode_bits = static_cast<uint8_t>(mode);

    // Clear CPOL/CPHA bits, then apply selected mode
    SPCR = (SPCR & ~((1 << CPOL) | (1 << CPHA))) | mode_bits;
}

uint8_t SPI::spi_transfer(uint8_t data)
{
    SPDR = data;

    while (!(SPSR & (1 << SPIF))) {
    }

    return SPDR;
}

void SPI::cs_low()
{
    PORTB &= ~(1 << PB2);
}

void SPI::cs_high()
{
    PORTB |= (1 << PB2);
}
