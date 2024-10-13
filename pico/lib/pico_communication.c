#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "receive_fifo.pio.h"
#include "transmit_fifo.pio.h"

#include "../../common/protocols.h"
#include "../../common/crc8.h"
#include "pico_communication.h"
#include "command_dispatch.h"
#include "sd_block_device.h"


PIO_state* init_pio(void) {
    stdio_init_all();

    printf("pico booting up pack!\n");
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_put(LED_PIN, 1);
    sleep_ms(250);
    gpio_put(LED_PIN, 0);
    sleep_ms(250);
    gpio_put(LED_PIN, 1);
    sleep_ms(250);
    gpio_put(LED_PIN, 0);
    sleep_ms(10);

    int p;
    for(p = 0; p < 2; ++p) {

        PIO pio = (p == 0) ? pio0 : pio1;
        for(int s = 0; s < 2; ++s) {
            printf("rp2.PIO(%d).state_machine(%d).pio_sm_is_claimed(): %s\n",p, s, pio_sm_is_claimed(pio, s) ? "true" : "false");
        }
    }
 
    PIO_state *pio_state = (PIO_state *)malloc(sizeof(PIO_state));
    if (pio_state == NULL) {
        printf("Failed to allocate memory for PIO_state\n");
        return pio_state; // Exit or handle the error
    }
    pio_state->pio = pio1;
    pio_state->rx_sm = pio_claim_unused_sm(pio_state->pio, true);
    pio_state->tx_sm = pio_claim_unused_sm(pio_state->pio, true);

    if (pio_state->rx_sm == -1) {
        printf("Error: Failed to claim a state machine for RX\n");
        pio_state->tx_sm = -1;
        return pio_state;;  // or handle error appropriately
    }

    if (pio_state->tx_sm == -1) {
        printf("Error: Failed to claim a state machine for TX\n");
        return pio_state;;  // or handle error appropriately
    }

    uint tx_offset = pio_add_program(pio_state->pio, &transmit_fifo_program);
    if (tx_offset == -1) {
        printf("Error: Failed to load transmit FIFO program\n");
        return pio_state;
    }

    uint rx_offset = pio_add_program(pio_state->pio, &receive_fifo_program);
    if (rx_offset == -1) {
        printf("Error: Failed to load receive FIFO program\n");
        return pio_state;
    }

    const uint RX_DATA_READY = 22;
    gpio_init(RX_DATA_READY);
    gpio_set_dir(RX_DATA_READY, GPIO_IN);

    const uint RX_TAKEN_PIN = 28;
    gpio_init(RX_TAKEN_PIN);
    gpio_set_dir(RX_TAKEN_PIN, GPIO_OUT);
    gpio_put(RX_TAKEN_PIN, 1);

    // clear RX state machine
    pio_sm_set_enabled(pio_state->pio, pio_state->rx_sm, false);
    pio_sm_clear_fifos(pio_state->pio, pio_state->rx_sm);
    pio_sm_restart(pio_state->pio, pio_state->rx_sm);

    // clear TX state machine
    pio_sm_set_enabled(pio_state->pio, pio_state->tx_sm, false);
    pio_sm_clear_fifos(pio_state->pio, pio_state->tx_sm);
    pio_sm_restart(pio_state->pio, pio_state->tx_sm);

    //for 1 MHz signal, sample PIO at 4MHz for clarity of signal
    //float target_freq = 4.0e6;  // 4MHz in Hz
    float target_freq = 4.0e6; 
    //equal to 125MHz system clock / 4MHz sampling rate
    float clkdiv = clock_get_hz(clk_sys) / target_freq; 

    printf("clock divider: %2.2f\n", clkdiv);

    printf("pico initing PIO\n");
    receive_fifo_init(pio_state->pio, pio_state->rx_sm, rx_offset, clkdiv);
    transmit_fifo_init(pio_state->pio, pio_state->tx_sm, tx_offset, clkdiv);
    return pio_state;
}

void process_incoming_commands(SDState *sd_state, PIO_state *pio_state) {
    while (true) {
        Payload *payload = (Payload*)malloc(sizeof(Payload));
        if (payload == NULL) {
            printf("Error: Memory allocation failed for payload\n");
            return;
        }
        memset(payload, 0, sizeof(Payload));
        receive_command_payload(pio_state, payload);
        Payload *response = dispatch_command(sd_state, pio_state, payload); 
        ResponseStatus status = transmit_response(pio_state, response);
        if (status != STATUS_OK) {
            printf("Error: Command dispatch failed\n");
        }
        free(payload->params);
        free(payload->data);
        free(payload);
        free(response->params);
        free(response->data);
        free(response);
    }
}

void receive_command_payload(PIO_state *pio_state, Payload *payload) {
    
    receive_command(pio_state, payload);
    while (is_valid_command_crc8(payload) ) {
        pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, false);  //send a CRC failure Response
        receive_command(pio_state, payload);    
    }
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, true);  //send a CRC success Response

    
    receive_data(pio_state, payload);
    while ( is_valid_data_crc8(payload) ) {
        pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, false);  //send a CRC failure Response
        receive_data(pio_state, payload);
    }
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, true);  //send a CRC success Response
}

void receive_command(PIO_state *pio_state, Payload *payload) {
    payload->protocol = (V9KProtocol) pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    payload->params_size = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    payload->command = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    payload->params = malloc(payload->params_size);
    if (payload->params == NULL) {
        printf("Error: Memory allocation failed for payload->params buffer\n");
        return;
    }
    for(int i = 0; i < payload->params_size; ++i) {
        payload->params[i] = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    }
    payload->command_crc = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
}

void receive_data(PIO_state *pio_state, Payload *payload) {
    uint8_t high_byte = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    uint8_t low_byte = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    payload->data_size = (high_byte << 8) | low_byte;
    
    payload->data = malloc(payload->data_size);
    if (payload->data == NULL) {
        printf("Error: Memory allocation failed for payload->data buffer\n");
        return;
    }
    for(int i = 0; i < payload->data_size; ++i) {
        payload->data[i] = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    }
    payload->data_crc = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
}

ResponseStatus transmit_response(PIO_state *pio_state, Payload *payload) {
    create_command_crc8(payload);
    create_data_crc8(payload);
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->protocol);
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->params_size);
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->command);
    for(int i = 0; i < payload->params_size; ++i) {
        pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->params[i]);
    }
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->command_crc);

    uint8_t high_byte = (payload->data_size >> 8) & 0xFF;
    uint8_t low_byte = payload->data_size & 0xFF;
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, high_byte);
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, low_byte);
    for(int i = 0; i < payload->data_size; ++i) {
        pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->data[i]);
    }
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->data_crc);
    return STATUS_OK;
}