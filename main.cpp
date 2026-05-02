#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/interrupt.h>
//#include <avr/eeprom.h>
#include "utils/StaticVector.hpp"
#include "modules/Timer/Timer.hpp"
#include "modules/SPI/SPI.hpp"

#define UART_BAUD 9600
#define UBRR_VALUE ((F_CPU / 16 / UART_BAUD) - 1)

static int uart_putchar(char c, FILE* stream);

static FILE uart_stdout;

static void uart_init()
{
    UBRR0H = static_cast<uint8_t>(UBRR_VALUE >> 8);
    UBRR0L = static_cast<uint8_t>(UBRR_VALUE);

    // Enable transmitter
    UCSR0B = (1 << TXEN0);

    // 8 data bits, 1 stop bit, no parity
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    fdev_setup_stream(&uart_stdout, uart_putchar, nullptr, _FDEV_SETUP_WRITE);
    stdout = &uart_stdout;
}

static int uart_putchar(char c, FILE* stream)
{
    (void)stream;

    if (c == '\n') {
        uart_putchar('\r', stream);
    }

    while (!(UCSR0A & (1 << UDRE0))) {
    }

    UDR0 = c;
    return 0;
}

uint16_t get_stack_ptr()
{
  uint8_t sreg = SREG;
  cli();
  uint8_t low = SPL;
  uint8_t high = SPH;
  SREG = sreg;
  return ((static_cast<uint16_t>(high) << 8) | low);
}

void foo()
{
  uint8_t test = 12;
  uint8_t *tptr = &test;
  *tptr = 11;
  printf("[foo]: SP=0x%04X\n", get_stack_ptr());
}


int main()
{
    uart_init();
    Timer::init();
    SPI spi;
    spi.spi_init();

    DDRB |= (1 << PB5);
    printf("program start\n");
  /*
    constexpr uint8_t _N = 32;
    StaticVector<long long, _N> values;
    for(size_t i = 0; i < _N; ++i)
    {
      values[i] = i*10;
    }

    printf("values size=%u\n", values.size());
    printf("values[0]=%lld\n", values[0]);
    printf("values[1]=%lld\n", values[1]);
*/
    unsigned long counter = 0;

    while (true)
    {
      uint16_t start = Timer::ticks();
      PORTB ^= (1 << PB5);

      uint16_t end = Timer::ticks();
      // uint16_t subtraction handles wraparound correctly
      uint16_t elapsed_ticks = end - start;
      uint32_t elapsed_us = Timer::micros_from_ticks(elapsed_ticks);
      printf("SP=0x%04X\n", get_stack_ptr());
      foo();
      printf("blink counter=%lu\n", counter++);

      printf("loop (nodelay) %lu us\n", elapsed_us);

      _delay_ms(500);

    }
}
