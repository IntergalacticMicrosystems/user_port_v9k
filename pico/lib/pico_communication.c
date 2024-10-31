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

#define __no_inline_not_in_flash_func(read_burst_from_pio_fifo) __noinline __not_in_flash_func(read_burst_from_pio_fifo)


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
    gpio_put(LED_PIN, 1);

    const uint RX_DATA_READY = 27;
    gpio_init(RX_DATA_READY);
    gpio_set_dir(RX_DATA_READY, GPIO_IN);

    const uint RX_TAKEN_PIN = 28;
    gpio_init(RX_TAKEN_PIN);
    gpio_set_dir(RX_TAKEN_PIN, GPIO_OUT);
    gpio_put(RX_TAKEN_PIN, 1);

    const uint TX_DATA_READY = 26;
    gpio_init(TX_DATA_READY);
    gpio_set_dir(TX_DATA_READY, GPIO_OUT);
    gpio_put(TX_DATA_READY, 1);
    gpio_set_function(TX_DATA_READY, GPIO_FUNC_PIO1);

    const uint TX_DATA_TAKEN = 22;
    gpio_init(TX_DATA_TAKEN);
    gpio_set_dir(TX_DATA_TAKEN, GPIO_IN);

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
    float target_freq = 30.0e6; 
    //equal to 125MHz system clock / 4MHz sampling rate
    float clkdiv = clock_get_hz(clk_sys) / target_freq; 

    printf("clock divider: %2.2f\n", clkdiv);

    printf("pico initing PIO\n");
    receive_fifo_init(pio_state->pio, pio_state->rx_sm, rx_offset, clkdiv);
    transmit_fifo_init(pio_state->pio, pio_state->tx_sm, tx_offset, clkdiv);
    return pio_state;
}

void wait_for_startup_handshake(PIO_state *pio_state) {
    printf("Waiting for startup handshake\n");
    while (true) {
        uint8_t handshake = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
        printf("Received byte %d\n", handshake);
        if (handshake == STARTUP_HANDSHAKE) {
            printf("Startup handshake received\n");
            pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, HANDSHAKE_RESPONSE);
            printf("Sent handshake response\n");
            // Handshake is considered complete
            return;

        } else {
            printf("Bad handshake response received: %d\n", handshake);
        }
    }
}

void process_incoming_commands(SDState *sd_state, PIO_state *pio_state) {
    printf("Processing incoming commands\n");
    while (true) {
        Payload *payload = (Payload*)malloc(sizeof(Payload));
        if (payload == NULL) {
            printf("Error: Memory allocation failed for payload\n");
            return;
        }
        memset(payload, 0, sizeof(Payload));
        ResponseStatus outcome = receive_command_payload(pio_state, payload);
        if (outcome != STATUS_OK) {
            printf("Error: Command payload reception failed %d\n", outcome);
            free(payload->params);
            free(payload->data);
            free(payload);
            continue;
        }
        Payload *response = dispatch_command(sd_state, pio_state, payload); 
        ResponseStatus status = transmit_response(pio_state, response);
        if (status != STATUS_OK) {
            printf("Error: Command dispatch failed\n");
        }       
        printf("Receive command Payload successfully\n");
        free(payload->params);
        free(payload->data);
        free(payload);
        free(response->params);
        free(response->data);
        free(response);
    }
}

uint16_t receive_utf16(PIO_state *pio_state) {
    uint8_t high_byte = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    uint8_t low_byte = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);   
    return (high_byte << 8) | low_byte;
}

ResponseStatus receive_command_payload(PIO_state *pio_state, Payload *payload) {

    printf("Waiting for incoming command\n");
    ResponseStatus outcome = receive_command_packet(pio_state, payload);
    if (outcome != STATUS_OK) {
        printf("Error: Command packet reception failed %d\n", outcome);
        return outcome;  
    } 
    printf("Valid CRC on command packet, receiving data packet\n");
    outcome = receive_data_packet(pio_state, payload);
    if (outcome != STATUS_OK) {
        printf("Error: Data packet reception failed %d\n", outcome);
        return outcome;
    }
    printf("Valid CRC on data packet\n");
    printf("Command payload received successfully\n");
    return outcome;
}

void read_burst_from_pio_fifo(PIO pio, uint sm, uint8_t *receiveData, uint32_t loop_size) {
    int count = 0;
    for (int i = 0; i < loop_size; ++i) {
        receiveData[i]= pio_sm_get_blocking(pio, sm);
    }
}

ResponseStatus receive_command_packet(PIO_state *pio_state, Payload *payload) {
    printf("Waiting for command packet\n");
    payload->protocol = (V9KProtocol) pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    if (payload->protocol == HANDSHAKE) {
        printf("Handshake protocol received instead of command_packet, retrying\n");
        for (int i = 0; i < 3; ++i) {
            payload->protocol = (V9KProtocol) pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
            if (payload->protocol != HANDSHAKE) {
                break;
            }
        }
    }
    payload->command = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm); 
    payload->params_size = receive_utf16(pio_state);
    payload->params = malloc(payload->params_size);
    if (payload->params == NULL) {
        printf("Error: Memory allocation failed for payload->params buffer\n");
        sendResponseStatus(pio_state, MEMORY_ALLOCATION_ERROR);
        return MEMORY_ALLOCATION_ERROR;
    }
    printf("Protocol: %d, Command: %d\n", payload->protocol, payload->command);
    printf("Recieving command parameters, size: %d\n", payload->params_size);
    read_burst_from_pio_fifo(pio_state->pio, pio_state->rx_sm, payload->params, payload->params_size);
    payload->command_crc = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    printf("Done getting command packet %d\n", payload->command_crc);
    if ( !is_valid_command_crc8(payload) ) {
        sendResponseStatus(pio_state, INVALID_CRC);  //send a CRC failure Response   
        printf("Invalid CRC on command packet\n");
        return INVALID_CRC;
    }
    printf("Valid CRC on command packet\n");
    sendResponseStatus(pio_state, STATUS_OK);  //send a CRC success Response
    return STATUS_OK;
}

ResponseStatus receive_data_packet(PIO_state *pio_state, Payload *payload) {
    printf("Waiting for data packet\n");
    payload->data_size = receive_utf16(pio_state);
    printf("Data size: %d\n", payload->data_size);
    payload->data = malloc(payload->data_size);
    if (payload->data == NULL) {
        printf("Error: Memory allocation failed for payload->data buffer\n");
        sendResponseStatus(pio_state, MEMORY_ALLOCATION_ERROR);
        return MEMORY_ALLOCATION_ERROR;
    }
    printf("Receiving data buffer\n");
    read_burst_from_pio_fifo(pio_state->pio, pio_state->rx_sm, payload->data, payload->data_size);
    
    printf("Receiving data buffer completed\n");
    payload->data_crc = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    printf("Received CRC, Done getting data packet\n");
    if ( !is_valid_data_crc8(payload) ) {
        sendResponseStatus(pio_state, INVALID_CRC);  //send a CRC failure Response
        printf("Invalid CRC on data packet\n");
        return INVALID_CRC;
    }
    printf("Valid CRC on data packet\n");
    sendResponseStatus(pio_state, STATUS_OK);  //send a CRC success Response
    return STATUS_OK;
}

void transmit_utf16(PIO_state *pio_state, uint16_t value) {
    uint8_t high_byte = (value >> 8) & 0xFF;
    uint8_t low_byte = value & 0xFF;
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, high_byte);
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, low_byte);
}

void sendResponseStatus(PIO_state *pio_state, ResponseStatus status) {
    uint8_t status_value = (uint8_t)status;  // Cast enum to uint8_t
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, status_value);
}

ResponseStatus transmit_response(PIO_state *pio_state, Payload *payload) {
    printf("Transmitting response\n");
    create_command_crc8(payload);
    create_data_crc8(payload);
    printf("Transmitting protocol: %d and command: %d\n", payload->protocol, payload->command);
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->protocol);
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->command);
    transmit_utf16(pio_state, payload->params_size);
    printf("Transmitting command parameters, size: %d\n", payload->params_size);
    for(int i = 0; i < payload->params_size; ++i) {
        pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->params[i]);
    }
    printf("Transmitting command CRC\n");
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->command_crc);
    printf("Waiting for CRC value\n");
    uint8_t crc_outcome = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    if (crc_outcome != STATUS_OK) {
        printf("Error: CRC or other failure on command portion of payload\n");
        return crc_outcome;
    }
    printf("Transmitting data packet\n");
    transmit_utf16(pio_state, payload->data_size);
    printf("Transmitting data buffer\n");
    for(int i = 0; i < payload->data_size; ++i) {
        pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->data[i]);
    }
    printf("transmitting data CRC\n");
    pio_sm_put_blocking(pio_state->pio, pio_state->tx_sm, payload->data_crc);

    printf("Waiting for CRC value\n");
    crc_outcome = pio_sm_get_blocking(pio_state->pio, pio_state->rx_sm);
    if (crc_outcome != STATUS_OK) {
        printf("Error: CRC or other failure on data portion of payload\n");
        return crc_outcome;
    }
    printf("Response transmitted successfully\n");
    return STATUS_OK;
}