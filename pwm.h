#pragma once

#include "defines.h"

extern byte ampl, period;
extern byte ph_u, ph_v, ph_w;
extern byte flags;

extern uint16_t delay_step;
extern uint8_t delay_num;

#define FLAG_GO  (1<<0)
#define FLAG_POT (1<<1)

#define MAX_PWM 100
