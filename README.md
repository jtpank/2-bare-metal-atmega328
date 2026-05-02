1. sudo apt-get install gcc-avr
2.  avr-gcc -mmcu=atmega328p -c blink.c
 this compiles your blink.o
3. sudo apt install avrdude avrdude-doc 
4. sudo apt install avr-libc
After install avr-libc, we can run:
avr-gcc -mmcu=atmega328p blink.c

to get an exectuable!


5. Now we can flash with avrdude:
avrdude -v -c /etc/avrdude.conf -p atmega328p -c stk500v1 -P /dev/ttyUSB0 -b 19200 -U flash:w:a.out:e

Now we can convert the ELF exe to a hex file using avr-objcopy, reducing the file size:

compile with -Os option for optimization!
avr-gcc -mmcu=atmega328p blink.c
avr-objcopy -O ihex -j.text -j.data a.out a-small.hex
avrdude -v -c /etc/avrdude.conf -p atmega328p -c stk500v1 -P /dev/ttyUSB0 -b 19200 -U flash:w:a-small.hex:i

If using the arduino bootloader:

avrdude -v -C /etc/avrdude.conf \
  -p atmega328p \
  -c arduino \
  -P /dev/ttyACM0 \
  -b 115200 \
  -U flash:w:a-small.hex:i


For C++ scripts:
avr-g++ \
  -std=c++11 \
  -mmcu=atmega328p \
  -DF_CPU=16000000UL \
  -Os \
  -Wall \
  -Wextra \
  -fno-exceptions \
  -fno-rtti \
  main.cpp \
  -o main.elf

