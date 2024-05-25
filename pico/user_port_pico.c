#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "receive_fifo.pio.h"
#include "transmit_fifo.pio.h"
#include "pico_communication.c"

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

    printf("strobbing GPIO 26!\n");
    const uint DATA_READY_PIN = 26;
    gpio_init(DATA_READY_PIN);
    gpio_set_dir(DATA_READY_PIN, GPIO_OUT);
    gpio_put(DATA_READY_PIN, 1);
    sleep_us(100);

    PIO pio = pio0; 
    uint tx_offset = pio_add_program(pio, &transmit_fifo_program);
     if (tx_offset == -1) {
        printf("Error: Failed to load transmit FIFO program\n");
        return 1;
    }

    uint rx_offset = pio_add_program(pio, &receive_fifo_program);
     if (rx_offset == -1) {
        printf("Error: Failed to load receive FIFO program\n");
        return 1;
    }

    uint rx_data_ready = 22;
    gpio_init(rx_data_ready);
    gpio_set_dir(rx_data_ready, GPIO_IN);

    uint rx_taken_pin = 28;
    gpio_init(rx_taken_pin);
    gpio_set_dir(rx_taken_pin, GPIO_OUT);
    gpio_put(rx_taken_pin, 1);

   
    uint rx_sm = pio_claim_unused_sm(pio, true);
    if (rx_sm == -1) {
        printf("Error: Failed to claim a state machine for RX\n");
        return 1;  // or handle error appropriately
    }

    // clear RX state machine
    pio_sm_set_enabled(pio, rx_sm, false);
    pio_sm_clear_fifos(pio, rx_sm);
    pio_sm_restart(pio, rx_sm);

    // clear TX state machine
    uint tx_sm = pio_claim_unused_sm(pio, true);
    if (tx_sm == -1) {
        printf("Error: Failed to claim a state machine for RX\n");
        return 1;  // or handle error appropriately
    }

    pio_sm_set_enabled(pio, tx_sm, false);
    pio_sm_clear_fifos(pio, tx_sm);
    pio_sm_restart(pio, tx_sm);

    //for 1 MHz signal, sample PIO at 4MHz for clarity of signal
    //float target_freq = 4.0e6;  // 4MHz in Hz
    float target_freq = 4.0e6; 
    //equal to 125MHz system clock / 4MHz sampling rate
    float clkdiv = clock_get_hz(clk_sys) / target_freq; 

    printf("clock divider: %2.2f\n", clkdiv);

    printf("pico initing PIO\n");
    receive_fifo_init(pio, rx_sm, rx_offset, clkdiv);
    transmit_fifo_init(pio, tx_sm, tx_offset, clkdiv);

    // Now, call the function to read data from the FIFO
    printf("pico sending outgoing data from PIO\n");
    send_to_pio_fifo(pio, tx_sm); 
    printf("pico reading incoming data to PIO\n");

    pio_sm_set_enabled(pio, tx_sm, true);
    read_from_pio_fifo(pio, rx_sm); // Example call, adjust `pio0` and `0` as necessary
    
    return 0;
}
