#ifndef PICO_COMMUNICATION_H
#define PICO_COMMUNICATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../../common/protocols.h"
#include "pico_common.h"
#include "sd_block_device.h"

PIO_state* init_pio(void);
void wait_for_startup_handshake(PIO_state *pio_state);
uint16_t receive_utf16(PIO_state *pio_state);
void transmit_utf16(PIO_state *pio_state, uint16_t value);
void sendResponseStatus(PIO_state *pio_state, ResponseStatus status);
ResponseStatus receive_command_payload(PIO_state *pio_state, Payload *payload);
ResponseStatus receive_command_packet(PIO_state *pio_state, Payload *payload);
ResponseStatus receive_data_packet(PIO_state *pio_state, Payload *payload);
void process_command(PIO_state *pio_state, Payload *payload);
void process_incoming_commands(SDState *sd_state, PIO_state *pio_state);
ResponseStatus transmit_response(PIO_state *pio_state, Payload *payload);

#endif