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

    bool debug = false;

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
    if (debug) cdprintf("Command CRC payload->protocol: %d\n", crc);

    // Process payload->params_size as two bytes
    crc = crc8_table[crc ^ ((payload->params_size >> 8) & 0xFF)];  // High byte
    if (debug) cdprintf("Command CRC payload->params_size high byte: %d\n", crc);
    crc = crc8_table[crc ^ (payload->params_size & 0xFF)];         // Low byte
    if (debug) cdprintf("Command CRC payload->params_size low byte: %d\n", crc);

    crc = crc8_table[crc ^ payload->command];
    if (debug) cdprintf("Command CRC payload->command: %d\n", crc);
    for (size_t i = 0; i < payload->params_size; i++) {
        crc = crc8_table[crc ^ payload->params[i]];
    }
    if (debug) cdprintf("Command CRC: %d\n", crc);
    payload->command_crc = crc;
}

void create_data_crc8(Payload *payload) {
    if (!initialized) {
        generate_crc8_table();
    }
    uint8_t crc = 0x00;  // Initial value

    if (debug) cdprintf("Data CRC payload->data_size: %d\n", payload->data_size);
    //if (debug) cdprintf("Data CRC payload->data: %s\n", payload->data);
    // Process payload->data_size as two bytes
    crc = crc8_table[crc ^ ((payload->data_size >> 8) & 0xFF)];  // High byte
    if (debug) cdprintf("Data CRC payload->data_size high byte: %d\n", crc);
    crc = crc8_table[crc ^ (payload->data_size & 0xFF)];         // Low byte
    if (debug) cdprintf("Data CRC payload->data_size low byte: %d\n", crc);

    for (size_t i = 0; i < payload->data_size; i++) {
        crc = crc8_table[crc ^ payload->data[i]];
    }
    if (debug) cdprintf("Data CRC: %d\n", crc);
    payload->data_crc = crc;
}

void create_payload_crc8(Payload *payload) {
    create_command_crc8(payload);
    create_data_crc8(payload);
}

bool is_valid_command_crc8(const Payload *payload) {
    if (!initialized) {
        generate_crc8_table();
    }
    uint8_t crc = 0x00;  // Initial value
    crc = crc8_table[crc ^ payload->protocol];
    if (debug) cdprintf("IsCommand CRC payload->protocol: %d\n", crc);
    
    // Process payload->params_size as two bytes
    crc = crc8_table[crc ^ ((payload->params_size >> 8) & 0xFF)];  // High byte
    if (debug) cdprintf("IsCommand CRC payload->params_size high byte: %d\n", crc);
    crc = crc8_table[crc ^ (payload->params_size & 0xFF)];         // Low byte
    if (debug) cdprintf("IsCommand CRC payload->params_size low byte: %d\n", crc);
    
    crc = crc8_table[crc ^ payload->command];
    if (debug) cdprintf("IsCommand CRC payload->command: %d\n", crc);
    for (size_t i = 0; i < payload->params_size; i++) {
        crc = crc8_table[crc ^ payload->params[i]];
    }
    if (debug) cdprintf("Command CRC payload->command_crc: %d calculated: %d\n", payload->command_crc, crc);
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

    // Process payload->data_size as two bytes
    crc = crc8_table[crc ^ ((payload->data_size >> 8) & 0xFF)];  // High byte
    if (debug) cdprintf("IsData CRC payload->data_size high byte: %d\n", crc);
    crc = crc8_table[crc ^ (payload->data_size & 0xFF)];         // Low byte
    if (debug) cdprintf("IsData CRC payload->data_size low byte: %d\n", crc);

    for (size_t i = 0; i < payload->data_size; i++) {
        crc = crc8_table[crc ^ payload->data[i]];
    }
    if (debug) cdprintf("Data CRC payload->data_crc: %d calculated: %d\n", payload->data_crc, crc);
    if ( payload->data_crc == crc) {
        return true;
    } else {
        return false;
    }
}
