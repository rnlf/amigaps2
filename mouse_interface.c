// amigaps2
//
// Copyright (c) 2018 Florian Kesseler
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the MIT license. See LICENSE for details.


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "mouse_interface.h"
#include "ps2.h"


#define MOUSE_BUTTON_PORT    PORTB
#define MOUSE_BUTTON_PIN_SHIFT 0
#define MOUSE_BUTTON_MASK (0x07 << MOUSE_BUTTON_PIN_SHIFT)

#define MOUSE_PULSE_PORT PORTC
#define MOUSE_PIN_H_MASK 0x0C
#define MOUSE_PIN_V_MASK 0x03


void
mouse_init(void) {
  DDRB  |= MOUSE_BUTTON_MASK;
  PORTB |= MOUSE_BUTTON_MASK;

  DDRC  |= MOUSE_PIN_H_MASK | MOUSE_PIN_V_MASK;
  PORTC |= MOUSE_PIN_H_MASK | MOUSE_PIN_V_MASK;

  TCCR0A = 2;
  TCCR0B = 3;
  OCR0A = 200;
  TIMSK0 = (1<<OCIE0A);

  TCCR2A = 2;
  TCCR2B = 4;
  OCR2A = 200;
  TIMSK2 = (1<<OCIE2A);
}


static uint8_t v_dir = 0xff;
static uint8_t h_step = 0;
static uint8_t h_dir = 0xff;
static uint8_t h_counter = 0;
static uint8_t v_counter = 0;
static uint8_t v_step = 0;


#define LUT(x) ((uint8_t) (249.0f * (1.0f - ((float)x - 1.0f)/254.0f)))
static const uint8_t timer_lut[] = {
  0xff,     LUT(1),   LUT(2),   LUT(3),   LUT(4),  LUT(5),    LUT(6),   LUT(7),   LUT(8),   LUT(9),   LUT(10),  LUT(11),  LUT(12),  LUT(13),  LUT(14),  LUT(15),
  LUT(16),  LUT(17),  LUT(18),  LUT(19),  LUT(20),  LUT(21),  LUT(22),  LUT(23),  LUT(24),  LUT(25),  LUT(26),  LUT(27),  LUT(28),  LUT(29),  LUT(30),  LUT(31),
  LUT(32),  LUT(33),  LUT(34),  LUT(35),  LUT(36),  LUT(37),  LUT(38),  LUT(39),  LUT(40),  LUT(41),  LUT(42),  LUT(43),  LUT(44),  LUT(45),  LUT(46),  LUT(47),
  LUT(48),  LUT(49),  LUT(50),  LUT(51),  LUT(52),  LUT(53),  LUT(54),  LUT(55),  LUT(56),  LUT(57),  LUT(58),  LUT(59),  LUT(60),  LUT(61),  LUT(62),  LUT(63),
  LUT(64),  LUT(65),  LUT(66),  LUT(67),  LUT(68),  LUT(69),  LUT(70),  LUT(71),  LUT(72),  LUT(73),  LUT(74),  LUT(75),  LUT(76),  LUT(77),  LUT(78),  LUT(79),
  LUT(80),  LUT(81),  LUT(82),  LUT(83),  LUT(84),  LUT(85),  LUT(86),  LUT(87),  LUT(88),  LUT(89),  LUT(90),  LUT(91),  LUT(92),  LUT(93),  LUT(94),  LUT(95),
  LUT(96),  LUT(97),  LUT(98),  LUT(99),  LUT(100), LUT(101), LUT(102), LUT(103), LUT(104), LUT(105), LUT(106), LUT(107), LUT(108), LUT(109), LUT(110), LUT(111),
  LUT(112), LUT(113), LUT(114), LUT(115), LUT(116), LUT(117), LUT(118), LUT(119), LUT(120), LUT(121), LUT(122), LUT(123), LUT(124), LUT(125), LUT(126), LUT(127),
  LUT(128), LUT(129), LUT(130), LUT(131), LUT(132), LUT(133), LUT(134), LUT(135), LUT(136), LUT(137), LUT(138), LUT(139), LUT(140), LUT(141), LUT(142), LUT(143),
  LUT(144), LUT(145), LUT(146), LUT(147), LUT(148), LUT(149), LUT(150), LUT(151), LUT(152), LUT(153), LUT(154), LUT(155), LUT(156), LUT(157), LUT(158), LUT(159),
  LUT(160), LUT(161), LUT(162), LUT(163), LUT(164), LUT(165), LUT(166), LUT(167), LUT(168), LUT(169), LUT(170), LUT(171), LUT(172), LUT(173), LUT(174), LUT(175),
  LUT(176), LUT(177), LUT(178), LUT(179), LUT(180), LUT(181), LUT(182), LUT(183), LUT(184), LUT(185), LUT(186), LUT(187), LUT(188), LUT(189), LUT(190), LUT(191),
  LUT(192), LUT(193), LUT(194), LUT(195), LUT(196), LUT(197), LUT(198), LUT(199), LUT(200), LUT(201), LUT(202), LUT(203), LUT(204), LUT(205), LUT(206), LUT(207),
  LUT(208), LUT(209), LUT(210), LUT(211), LUT(212), LUT(213), LUT(214), LUT(215), LUT(216), LUT(217), LUT(218), LUT(219), LUT(220), LUT(221), LUT(222), LUT(223),
  LUT(224), LUT(225), LUT(226), LUT(227), LUT(228), LUT(229), LUT(230), LUT(231), LUT(232), LUT(233), LUT(234), LUT(235), LUT(236), LUT(237), LUT(238), LUT(239),
  LUT(240), LUT(241), LUT(242), LUT(243), LUT(244), LUT(245), LUT(246), LUT(247), LUT(248), LUT(249), LUT(250), LUT(251), LUT(252), LUT(253), LUT(254), LUT(255)
};
#undef LUT


void
mouse_process(void) {
  MOUSE_BUTTON_PORT = (MOUSE_BUTTON_PORT & ~MOUSE_BUTTON_MASK) | (((~ps2_movement_data.flags) & 0x07) << MOUSE_BUTTON_PIN_SHIFT);

  if(ps2_movement_data.flags & PS2_X_NEGATIVE) {
    h_dir = 0xff;
    // I am not sure the mouse will ever return -256, but in theory it could
    // let's just treat it as -255
    if(ps2_movement_data.distance_x == 0) {
      ps2_movement_data.distance_x = 1;
    }
    h_counter = ~ps2_movement_data.distance_x + 1;
  } else {
    h_counter = ps2_movement_data.distance_x;
    h_dir = 0x01;
  }

  OCR2A = timer_lut[h_counter];


  if(ps2_movement_data.flags & PS2_Y_NEGATIVE) {
    v_dir = 0x01;
    // I am not sure the mouse will ever return -256, but in theory it could
    // let's just treat it as -255
    if(ps2_movement_data.distance_y == 0) {
      ps2_movement_data.distance_y = 1;
    }
    v_counter = ~ps2_movement_data.distance_y + 1;
  } else {
    v_counter = ps2_movement_data.distance_y;
    v_dir = 0xff;
  }

  OCR0A = timer_lut[v_counter];
}


const static uint8_t v_steps[] = {
  0x01, 0x03, 0x02, 0x00
};


const static uint8_t h_steps[] = {
  0x04, 0x0C, 0x08, 0x00
};


ISR(TIMER0_COMPA_vect) {
  if(v_counter > 0) {
    MOUSE_PULSE_PORT = (MOUSE_PULSE_PORT & ~MOUSE_PIN_V_MASK) | v_steps[v_step];
    v_step = (v_step + v_dir) & 0x03;
    --v_counter;
  }
}


ISR(TIMER2_COMPA_vect) {
  if(h_counter > 0) {
    MOUSE_PULSE_PORT = (MOUSE_PULSE_PORT & ~MOUSE_PIN_H_MASK) | h_steps[h_step];
    h_step = (h_step + h_dir) & 0x03;
    --h_counter;
  }
}
