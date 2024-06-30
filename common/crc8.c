#include <stdint.h>
#include <stddef.h>

#include "protocols.h"
#include "crc8.h"

#define POLYNOMIAL 0x07

uint8_t crc8_table[256];
bool initialized = false;

void generateCrc8Table() {
    for (int i = 0; i < 256; i++) {
        uint8_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
        crc8_table[i] = crc;
    }
    initialized = true;
}

uint8_t crc8(const uint8_t *data, size_t len) {
    if (!initialized) {
        generateCrc8Table();
    }
    uint8_t crc = 0x00;  // Initial value
    for (size_t i = 0; i < len; i++) {
        crc = crc8_table[crc ^ data[i]];
    }
    return crc;
}

void createCommandCrc8(VictorCommand *cmd) {
    if (!initialized) {
        generateCrc8Table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ cmd->protocol];
    crc = crc8_table[crc ^ cmd->byte_count];
    crc = crc8_table[crc ^ cmd->command];
    for (size_t i = 0; i < cmd->byte_count; i++) {
        crc = crc8_table[crc ^ cmd->params[i]];
    }
    cmd->expected_crc = crc;
}

bool isValidCommandCrc8(const VictorCommand *cmd) {
     if (!initialized) {
        generateCrc8Table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ cmd->protocol];
    crc = crc8_table[crc ^ cmd->byte_count];
    crc = crc8_table[crc ^ cmd->command];
    for (size_t i = 0; i < cmd->byte_count; i++) {
        crc = crc8_table[crc ^ cmd->params[i]];
    }
    if ( cmd->expected_crc == crc) {
        return true;
    } else {
        return false;
    }
}

bool isValidDataCrc8(const Data *data) {
    if (!initialized) {
        generateCrc8Table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ data->byte_count];
    for (size_t i = 0; i < data->byte_count; i++) {
        crc = crc8_table[crc ^ data->buffer[i]];
    }
    if ( data->expected_crc == crc) {
        return true;
    } else {
        return false;
    }
}
