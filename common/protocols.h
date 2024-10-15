#include <stdint.h>
#include <stdbool.h>

#ifndef _V9KPROTOCOLS_H_
#define _V9KPROTOCOLS_H_
typedef enum { PROTOCOL_UNKNOWN, PICO_RESET, SD_BLOCK_DEVICE, STANDARD_RAM, DOS_INTERRUPT, BIOS_INTERRUPT,
        EXPANDED_RAM, CLOCK, PRINTER, SCSI_REQUEST, LOG_OUTPUT,
        NETWORK, FLOPPY, VGA_DISPLAY, SOUND } V9KProtocol;

// Define status codes
typedef enum {
    STATUS_OK = 0,
    GENERAL_ERROR = 1,
    TIMEOUT = 2,
    INVALID_PROTOCOL = 3,
    INVALID_COMMAND = 4,
    INVALID_PARAMS = 5, 
    INVALID_DATA = 6,
    INVALID_CRC = 7,
    FILE_NOT_FOUND = 8,
    FILE_SEEK_ERROR = 9,
    MEMORY_ALLOCATION_ERROR = 10,
  // Additional status codes as needed
} ResponseStatus;

typedef struct {
    V9KProtocol protocol;
    uint8_t command;
    ResponseStatus status;    // Use STATUS_OK for requests; set by responses
    uint16_t params_size;     // Size of params
    uint16_t data_size;       // Size of data
    uint8_t *params;          // Command parameters
    uint8_t *data;            // Data payload
    uint8_t command_crc;      // CRC for the command portion of the payload
    uint8_t data_crc;         // CRC for the data portion of the payload
} Payload;

#endif /* _V9KPROTOCOLS_H_ */
