#include <stdint.h>
#include <stdbool.h>

#ifndef _V9KPROTOCOLS_H_
#define _V9KPROTOCOLS_H_
typedef enum { PROTOCOL_UNKNOWN, PICO_RESET, SD_BLOCK_DEVICE, STANDARD_RAM, DOS_INTERRUPT, BIOS_INTERRUPT,
        EXPANDED_RAM, RTC, PRINTER, SCSI_REQUEST, LOG_OUTPUT,
        NETWORK, FLOPPY, VGA_DISPLAY, SOUND } V9KProtocol;

// protocol request & response
typedef struct {
  V9KProtocol protocol;          
    uint8_t byte_count;   
    uint8_t command;        
    uint8_t *params;
    uint8_t expected_crc;
} VictorCommand;

typedef struct {        
    uint16_t byte_count;          
    uint8_t *buffer;
    uint8_t expected_crc;
} Data;

typedef struct {
    VictorCommand *cmd;
    Data *data;
} Payload;

#endif /* _V9KPROTOCOLS_H_ */
