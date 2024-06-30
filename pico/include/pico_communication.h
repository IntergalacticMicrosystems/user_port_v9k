#ifndef PICO_COMMUNICATION_H
#define PICO_COMMUNICATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../../common/protocols.h"
#include "pico_common.h"
#include "sd_block_device.h"

PIO_state* init_pio(void);
void generate_crc8_table();
void receive_command_payload(PIO_state *pio_state, Payload *payload);
void receive_command(PIO_state *pio_state, VictorCommand *cmd);
void receive_data(PIO_state *pio_state, Data *data);
void process_command(PIO_state *pio_state, Payload *payload);
void process_incoming_commands(SDState *sd_state, PIO_state *pio_state);

#endif