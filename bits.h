// amigaps2
//
// Copyright (c) 2018 Florian Kesseler
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the MIT license. See LICENSE for details.

#pragma once

#define set(port, bit, onoff) do { (port) = ((port) & ~(1<<(bit))) | ((onoff) << (bit)); } while(0)
#define sbi(port, bit) do { (port) = (port) | (1<<(bit)); } while(0)
#define cbi(port, bit) do { (port) = (port) & ~(1<<(bit)); } while(0)
#define trigger(port, bit) do { sbi(port, bit); cbi(port, bit); } while(0)

