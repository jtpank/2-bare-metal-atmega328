#ifndef TIMER_HPP
#define TIMER_HPP

#include <avr/io.h>
#include <stdint.h>

class Timer {
public:
    static void init();
    static uint16_t ticks();
    static uint32_t micros_from_ticks(uint16_t ticks);
};

#endif
