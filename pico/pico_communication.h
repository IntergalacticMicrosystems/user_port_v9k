#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../common/protocols.h"

typedef struct {
  PIO pio;
  uint rx_sm;
  uint tx_sm;
} PIO_state;

void receive_command_payload(PIO_state *pio_state, Payload *payload);
void receive_command(PIO_state *pio_state, Command *cmd);
void receive_data(PIO_state *pio_state, Data *data);
void process_incoming_commands(PIO_state *pio_state);
