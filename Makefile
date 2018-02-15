# amigaps2
#
# Copyright (c) 2018 Florian Kesseler
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the MIT license. See LICENSE for details.

CC = avr-gcc
CFLAGS = -mmcu=atmega328p -std=c99 -O3 -Wall
LDFLAGS = -mmcu=atmega328p

all: amigaps2.hex

clean:
	rm -f *.o *.hex *.elf

amigaps2.elf: main.o ps2.o mouse_interface.c
	$(CC) $(CFLAGS) -o $@ $^

amigaps2.hex: amigaps2.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

flash: amigaps2.hex
	avrdude -p m328p -c avrisp2 -U flash:w:amigaps2.hex:i
