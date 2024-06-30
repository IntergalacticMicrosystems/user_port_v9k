#ifndef PICO_COMMON_H
#define PICO_COMMON_H
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pico/multicore.h"

typedef struct {
  PIO pio;
  uint rx_sm;
  uint tx_sm;
} PIO_state;


#endif
