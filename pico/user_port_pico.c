#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/multicore.h"

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
    
    // Generate the CRC-8 lookup table
    generate_crc8_table();

    // Initialize PIO
    PIO_state *pio_state = init_pio();

    //Initialize the SD Card
    const char *directory = "/disks";
    SDState *sd_state = initializeSDState(directory);

    process_incoming_commands(sd_state, pio_state);

    return 0;

}
