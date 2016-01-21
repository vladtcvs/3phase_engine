#pragma once

#include "defines.h"

void tx_byte(byte c);
void setup_uart(unsigned long bitrate);
void print_int_hex(byte val);
void print_int_dec(uint16_t val);
