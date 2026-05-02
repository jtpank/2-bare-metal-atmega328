#!/usr/bin/env bash
set -euo pipefail

MCU="atmega328p"
F_CPU="16000000UL"
BAUD="9600"
PORT="/dev/ttyACM0"
ELF="main_cpp.elf"
HEX="main_cpp.hex"
LOG="serial_cpp.log"

SOURCES=(
  main.cpp
  modules/Timer/Timer.cpp
  modules/SPI/SPI.cpp
  modules/rf24/RF24.cpp
)

avr-g++ \
  -std=c++11 \
  -mmcu="$MCU" \
  -DF_CPU="$F_CPU" \
  -Imodules/SPI \
  -Imodules/Timer \
  -Imodules/rf24 \
  -Iutils \
  -Os \
  -Wall \
  -Wextra \
  -fno-exceptions \
  -fno-rtti \
  "${SOURCES[@]}" \
  -o "$ELF"

avr-objcopy \
  -O ihex \
  -R .eeprom \
  "$ELF" \
  "$HEX"

avrdude -v -C /etc/avrdude.conf \
  -p "$MCU" \
  -c arduino \
  -P "$PORT" \
  -b 115200 \
  -U flash:w:"$HEX":i

echo "Flashed successfully."
echo "Logging serial output from $PORT at $BAUD baud into $LOG"
echo "Press Ctrl+C to stop."

echo
echo "Memory usage:"
avr-size "$ELF"
echo
avr-size -C --mcu="$MCU" "$ELF"
echo

stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb -ixon -ixoff raw

cat "$PORT" | tee -a "$LOG"
