#include <stdint.h>
#include <stddef.h>

#include "protocols.h"
#include "crc8.h"

#define POLYNOMIAL 0x07

#ifdef __WATCOMC__
    // Code specific to OpenWatcom C compiler
    #include "cprint.h"     /* Console printing direct to hardware */
#endif

#ifdef __GNUC__
    #include <stdio.h>

    // Code specific to GNU ARM compiler (GCC)
    void cdprintf (char *msg, ...) {
        printf(msg);
    };
#endif

uint8_t crc8_table[256];
bool initialized = false;

void generate_crc8_table() {
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
        generate_crc8_table();
    }
    uint8_t crc = 0x00;  // Initial value
    for (size_t i = 0; i < len; i++) {
        crc = crc8_table[crc ^ data[i]];
    }
    return crc;
}

void create_command_crc8(Payload *payload) {
    if (!initialized) {
        generate_crc8_table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ payload->protocol];
    crc = crc8_table[crc ^ payload->params_size];
    crc = crc8_table[crc ^ payload->command];
    for (size_t i = 0; i < payload->params_size; i++) {
        crc = crc8_table[crc ^ payload->params[i]];
    }
    cdprintf("Command CRC: %d\n", crc);
    payload->command_crc = crc;
}

void create_data_crc8(Payload *payload) {
    if (!initialized) {
        generate_crc8_table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ payload->data_size];
    for (size_t i = 0; i < payload->data_size; i++) {
        crc = crc8_table[crc ^ payload->data[i]];
    }
    cdprintf("Data CRC: %d\n", crc);
    payload->data_crc = crc;
}

bool is_valid_command_crc8(const Payload *payload) {
    if (!initialized) {
        generate_crc8_table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ payload->protocol];
    crc = crc8_table[crc ^ payload->params_size];
    crc = crc8_table[crc ^ payload->command];
    for (size_t i = 0; i < payload->params_size; i++) {
        crc = crc8_table[crc ^ payload->params[i]];
    }
    cdprintf("Command CRC payload->command_crc: %d calculated: %d\n", payload->command_crc, crc);
    if ( payload->command_crc == crc) {
        return true;
    } else {
        return false;
    }
}

bool is_valid_data_crc8(const Payload *payload) {
    if (!initialized) {
        generate_crc8_table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ payload->data_size];
    for (size_t i = 0; i < payload->data_size; i++) {
        crc = crc8_table[crc ^ payload->data[i]];
    }
    cdprintf("Data CRC payload->data_crc: %d calculated: %d\n", payload->data_crc, crc);
    if ( payload->data_crc == crc) {
        return true;
    } else {
        return false;
    }
}
