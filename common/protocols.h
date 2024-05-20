#include <stdint.h>

#ifndef _V9KPROTOCOLS_H_
#define _V9KPROTOCOLS_H_
typedef enum { PICO_RESET, STANDARD_RAM, DOS_INTERRUPT, BIOS_INTERRUPT,
           DOS_DEVICE_DRIVER, EXPANDED_RAM, RTC, PRINTER, SCSI_REQUEST, LOG_OUTPUT,
           NETWORK, FLOPPY, VGA_DISPLAY, SOUND } protocol_type;

// protocol request & response
typedef struct {
  protocol_type protocol;          
  uint8_t byte_count;   
  uint8_t command;        
  uint8_t *params;
  uint8_t expected_crc;
} Command;

//The 
typedef struct {        
  uint16_t byte_count;          
  uint8_t *buffer;
  uint8_t expected_crc;
} Data;

typedef struct {
  Command *cmd;
  Data *data;
} payload

#endif /* _V9KPROTOCOLS_H_ */