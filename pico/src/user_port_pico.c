#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/multicore.h"

#include "user_port_pico.h"
#include "receive_fifo.pio.h"
#include "transmit_fifo.pio.h"
#include "pico_communication.h"
#include "sd_block_device.h"

// Assume pio0 is the PIO instance and sm is the state machine number
// This could be part of your main function or a dedicated function for handling PIO data
void read_from_pio_fifo(PIO pio, uint sm) {
    int count = 0;
    while (true) {
        uint32_t data = pio_sm_get_blocking(pio, sm);
        //printf("Received data: %d FIFO depth: %d\n", data, pio_sm_get_rx_fifo_level(pio, sm));
        if (++count % 100 == 0) {
            printf("Received data: %d FIFO depth: %d\n", data, pio_sm_get_rx_fifo_level(pio, sm));
        }

    }
}

void send_to_pio_fifo(PIO pio, uint sm) {
    uint8_t sendData[200]; // Example sendData to send
    for(int i = 0; i < 200; ++i) {
      sendData[i] = i; // Initialize sendData
    }

    for(int i = 0; i < 200; ++i) {
        bool warned = false;
        while (pio_sm_is_tx_fifo_full(pio, sm)) {
            if (! warned) {
                //printf("Warning: TX FIFO is full, waiting...\n");
            }
            warned = true;
            sleep_ms(1);  // Simple throttling
        }
        //printf("Sending data: %d FIFO depth: %d\n", sendData[i], pio_sm_get_tx_fifo_level(pio, sm));
        if (i % 100 == 0) {
            printf("Sending data: %d FIFO depth: %d\n", sendData[i], pio_sm_get_tx_fifo_level(pio, sm));
        }
        pio_sm_put_blocking(pio, sm, sendData[i]);
    }
}

int main(void) {
    stdio_init_all();

    uart_init(UART_ID, 115200);  // You can adjust the baud rate here
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // // Redirect stdout to UART
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    puts("User Port Pico Initializing...");
    // Initialize PIO
    PIO_state *pio_state = init_pio();

    //Initialize the SD Card
    const char *directory = "";
    SDState *sd_state = initialize_sd_state(directory);

    wait_for_startup_handshake(pio_state);
    process_incoming_commands(sd_state, pio_state);

    return 0;

}
