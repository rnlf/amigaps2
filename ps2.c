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

#define CLOCK_BIT  4
#define DATA_BIT   8
#define CLOCK_AND_DATA_BITS (CLOCK_BIT | DATA_BIT)
#define READ_MODE  0
#define WRITE_MODE 1
#define REQUEST_TO_SEND_DURATION 100
#define TIMEOUT 1000UL
#define TIMEOUT_OCR ((F_CPU) / (1024UL * (TIMEOUT)) - 1UL)


typedef enum DataMode {
  DATA_MODE_READ,
  DATA_MODE_SEND
} DataMode;


typedef struct  {
  uint8_t buf[3];
  uint8_t count;
} ReceiveBuffer;

static volatile DataMode mode = DATA_MODE_READ;
static volatile ReceiveBuffer receive_buf;
static volatile uint8_t  byte_buf;
static volatile uint8_t  counter;
static volatile uint8_t  parity;
static volatile uint8_t  timeout_counter;


PS2_MovementData ps2_movement_data;

void ps2_send_to_device(uint8_t byte);

bool
ps2_pump(void) {
  if(receive_buf.count != 3) {
    return false;
  }
  
  ps2_movement_data.flags      = receive_buf.buf[0];
  ps2_movement_data.distance_x = receive_buf.buf[1];
  ps2_movement_data.distance_y = receive_buf.buf[2];
  receive_buf.count = 0;

  return true;
}


int
ps2_init(void) {
  EICRA = 2; // xxxxxx10, Falling edge on D2 triggeres INT0
  EIMSK = 1; // xxxxxxx1, Exable external interrupt INT0

  DDRD  &= ~CLOCK_AND_DATA_BITS;

  // Enable pull-ups
  PORTD |= CLOCK_AND_DATA_BITS;

  TCCR1A =  0; // Disable PWM
  TCCR1B = 13; // Prescaler 1024
  TCCR1C =  0;

  // Bit-receive timeout
  OCR1AH = TIMEOUT >> 8;
  OCR1AL = TIMEOUT & 0xFF;
  TIMSK1 = (1<<OCIE1A);

  sei();

  // Wait for Basic Assurance Test result from mouse
  while(receive_buf.count == 0) {}
  if(receive_buf.buf[0] != PS2_BAT_SUCCESSFUL) {
    return PS2_ERROR;
  }


  // Wait for device ID and ignore it :)
  while(receive_buf.count == 1) {}

  // Send enable data reporting command
  ps2_send_to_device(PS2_COMMAND_ENABLE_DATA_REPORTING);

  // Wait for acknowledge for enable data reporting command
  while(receive_buf.count == 2) {}
  if(receive_buf.buf[2] != PS2_RESPONSE_ACKNOWLEDGE) {
    return PS2_ERROR;
  }

  return PS2_SUCCESS;
}


void
ps2_send_to_device(uint8_t byte) {
  PORTB ^= 1<<5;
  EIMSK = 0;
  cli();
  // Reset receive timeout
  TCNT1H = 0;
  TCNT1L = 0;

  byte_buf = byte;
  parity = 1;
  for(int i = 0; i < 8; ++i) {
    if(byte & 1) {
      parity = !parity;
    }
    byte = byte >> 1;
  }
  counter = 0;

  // Set output
  DDRD  |=  CLOCK_AND_DATA_BITS;
  // Pull down
  PORTD &= ~CLOCK_AND_DATA_BITS;

  _delay_us(REQUEST_TO_SEND_DURATION);
  mode = WRITE_MODE;

  // Reset receive timeout
  TCNT1H = 0;
  TCNT1L = 0;

  EIMSK = 1;
  sei();

  // Release clock line, re-enable pull-up
  DDRD  &= ~CLOCK_BIT;
  PORTD |=  CLOCK_BIT;
}


ISR(INT0_vect) {
  if(mode == READ_MODE) {
    uint8_t port = PIND; // Must happen ASAP
    
    switch(++counter) {
    case 1:                         // Start bit
      break;
    
    case 2: case 3: case 4: case 5: // Data bits
    case 6: case 7: case 8: case 9:
      // Bits are received LSB first, shift already-received ones down,
      // and add the new one as new MSB. After 8 received bits it all
      // fits!
      byte_buf = (byte_buf >> 1) | ((port & DATA_BIT) ? 0x80 : 0);
      // Doing this here allows us to process the byte 2 bits earlier
      // at the cost of missing bad transmissions. Reduces lag by 170us (wow!)
      if(counter == 9) {
        //receive_buffer.data[receive_buffer.write_pos++] = module_data.byte_buf;
        receive_buf.buf[receive_buf.count++] = byte_buf;
      }
      break;
    
    case 10:                         // Parity bits
      break;
    
    case 11:                        // Stop bit
      counter = 0;
      break;
    }

  } else if(mode == WRITE_MODE) {
    // Start bit was created in the send function, continue with 

    switch(++counter) {
    case 2: case 3: case 4: case 5: // Data bits
    case 6: case 7: case 8: case 9:
      // Shift out LSB first
      PORTD = (PORTD & ~DATA_BIT) | ((byte_buf & 1) ? DATA_BIT : 0);
      byte_buf = byte_buf >> 1;
      break;

    case 10:                         // Parity bit
      PORTD = (PORTD & ~DATA_BIT) | (parity ? DATA_BIT : 0);
      break;

    case 11:                        // Stop bit, just release data line and re-enable pull-up
      DDRD  &= ~DATA_BIT;
      PORTD |=  DATA_BIT;
      break;

    case 12:                        // Ack bit generated by device
      mode = READ_MODE;
      counter = 0;
      break;
    }
  }

  // Reset receive timeout
  TCNT1H = 0;
  TCNT1L = 0;
}


ISR(TIMER1_COMPA_vect) {
  // Receive timeout
  counter = 0;
  mode    = READ_MODE;

  // Inter-byte timeout of 5 milliseconds.
  // This erases the whole message every 5ms after the last bit was received.
  // We should never get close to that in the processingn delay.
  if(++timeout_counter > 5) {
    timeout_counter = 0;
    receive_buf.count = 0;
  }
}