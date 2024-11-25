#ifndef LOG_FUNCTIONS_H
#define LOG_FUNCTIONS_H

#include "pico_common.h"
#include "sd_block_device.h"

Payload* log_output(SDState *sdState, PIO_state *pio_state, Payload *payload);

#endif