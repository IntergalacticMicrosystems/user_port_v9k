#include <stdint.h>
#include <stdbool.h>

#ifndef _V9KPROTOCOLS_H_
#define _V9KPROTOCOLS_H_
#pragma pack(push, 1)
typedef enum { 
    PROTOCOL_UNKNOWN = 0, 
    PICO_RESET = 1, 
    SD_BLOCK_DEVICE = 2, 
    STANDARD_RAM = 3, 
    DOS_INTERRUPT = 4, 
    BIOS_INTERRUPT = 5,
    EXPANDED_RAM = 6, 
    CLOCK = 7, 
    PRINTER = 8, 
    SCSI_REQUEST = 9, 
    LOG_OUTPUT = 10,
    NETWORK = 11,
    FLOPPY = 12, 
    VGA_DISPLAY = 13, 
    SOUND = 14, 
    HANDSHAKE =15 
  } V9KProtocol;

#define MAX_IMG_FILES 9
#define MAX_PARTITIONS 16
#define FILENAME_MAX_LENGTH 260
#define SECTOR_SIZE 512
#define BUFFER_SECTOR_SIZE 10
#define STARTUP_HANDSHAKE 0x0F  // Handshake byte to start communication ASCII SI  (shift in)
#define HANDSHAKE_RESPONSE 0x0E // Handshake byte to acknowledge communication ASCII SO (shift out)

// Define status codes
typedef enum {
    STATUS_OK = 0,
    GENERAL_ERROR = 1,
    TIMEOUT = 2,
    INVALID_PROTOCOL = 3,
    INVALID_COMMAND = 4,
    INVALID_PARAMS = 5, 
    INVALID_DATA_SIZE = 6,
    INVALID_CRC = 7,
    FILE_NOT_FOUND = 8,
    FILE_SEEK_ERROR = 9,
    MEMORY_ALLOCATION_ERROR = 10,
    PORT_NOT_INITIALIZED = 11,
  // Additional status codes as needed
} ResponseStatus;

typedef struct {
    V9KProtocol protocol;
    uint8_t command;
    ResponseStatus status;    // Use STATUS_OK for requests; set by responses
    uint16_t params_size;     // Size of params
    uint16_t data_size;       // Size of data
    uint8_t *params;          // Command parameters
    #ifdef __WATCOMC__
    uint8_t far *data;            // Data payload
    #endif
    #ifdef __GNUC__
    uint8_t *data;            // Data payload
    #endif
    uint8_t command_crc;      // CRC for the command portion of the payload
    uint8_t data_crc;         // CRC for the data portion of the payload
} Payload;
#pragma pack(pop)

#endif /* _V9KPROTOCOLS_H_ */
