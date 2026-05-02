#include "Timer.hpp"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

void Timer::init()
{
    // Timer1 normal mode
    TCCR1A = 0;

    // Prescaler = 64
    // CS11 + CS10 = clk / 64
    //TCCR1B = (1 << CS11) | (1 << CS10);
    TCCR1B = (1 << CS10);

    // Start from zero
    TCNT1 = 0;
}

uint16_t Timer::ticks()
{
    return TCNT1;
}

uint32_t Timer::micros_from_ticks(uint16_t ticks)
{
    // 16MHz / prescaler (64) each tick is 4 us
    //return static_cast<uint32_t>(ticks) * 4UL;
    return (static_cast<uint32_t>(ticks) * 625UL) / 10UL;
}
