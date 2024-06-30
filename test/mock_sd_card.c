#include "mock_sd_card.h"
#include <string.h>

// Mock internal state
static bool initialized = false;
#define MAX_BLOCK 1024  // Define maximum blocks for simplicity
static uint8_t sd_card_data[MAX_BLOCK][512];  // Simulate SD card data storage

bool mock_sd_initialize(uint8_t unit, uint8_t partno) {
    // Simulate successful initialization
    initialized = true;
    return initialized;
}

int mock_sd_read(uint16_t unit, uint32_t block, uint8_t *buffer, uint16_t count) {
    if (!initialized || block >= MAX_BLOCK) {
        return -1;  // Simulate read error
    }
    // Simulate reading data from a "block" of the SD card
    memcpy(buffer, sd_card_data[block], 512 * count);  // Assuming 512 bytes per block
    return 0;  // Simulate successful read
}

int mock_sd_write(uint16_t unit, uint32_t block, const uint8_t *buffer, uint16_t count) {
    if (!initialized || block >= MAX_BLOCK) {
        return -1;  // Simulate write error
    }
    // Simulate writing data to a "block" of the SD card
    memcpy(sd_card_data[block], buffer, 512 * count);  // Assuming 512 bytes per block
    return 0;  // Simulate successful write
}

