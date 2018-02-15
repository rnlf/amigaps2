// amigaps2
//
// Copyright (c) 2018 Florian Kesseler
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the MIT license. See LICENSE for details.

#pragma once

#include <stdbool.h>

int ps2_init(void);
bool ps2_pump(void);

#define PS2_ERROR 1
#define PS2_SUCCESS 0

#define PS2_BAT_SUCCESSFUL 0xAA
#define PS2_COMMAND_ENABLE_DATA_REPORTING 0xF4
#define PS2_RESPONSE_ACKNOWLEDGE 0xFA

#define PS2_X_NEGATIVE    0x10
#define PS2_Y_NEGATIVE    0x20
#define PS2_LEFT_BUTTON   0x01
#define PS2_RIGHT_BUTTON  0x02
#define PS2_MIDDLE_BUTTON 0x04

typedef struct {
  uint8_t flags;
  uint8_t distance_x;
  uint8_t distance_y;
} PS2_MovementData;

extern PS2_MovementData ps2_movement_data;
