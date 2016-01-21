#pragma once

#include "defines.h"

extern byte ampl, period;
extern byte ph_u, ph_v, ph_w;
extern byte flags;

#define FLAG_GO  (1<<0)
#define FLAG_POT (1<<1)
