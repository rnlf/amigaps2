// amigaps2
//
// Copyright (c) 2018 Florian Kesseler
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the MIT license. See LICENSE for details.

#define F_CPU 16000000
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "ps2.h"
#include "mouse_interface.h"


void
init_hw(void) {
  // Disable all pull-ups
  DDRB  = 0x3C;
  DDRC  = 0x06;
  DDRD  = 0x02;

  PORTB = 0x00;
  PORTC = 0x06;
  PORTD = 0x00;
}


int
main(void) {
  init_hw();
  mouse_init();

  if(ps2_init() != PS2_SUCCESS) {
    PORTB |= 1<<5;
    for(;;);
  }
  
  uint8_t counter = 0;
  for(;;) {
    bool event = ps2_pump();
    if(event) {
      mouse_process();
      ++counter;
    }
  }
}

