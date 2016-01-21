#pragma once

#include "defines.h"

#define PHASES 36
#define FREQ_MAX ((uint16_t)488) /* 18e6/1024/36 */

extern byte ampl, period;
extern byte ph_u, ph_v, ph_w;
extern byte go;
