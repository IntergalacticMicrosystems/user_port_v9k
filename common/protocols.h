#include <stdint.h>
#include <stdbool.h>

#include <hardware/pio.h>

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
} Payload;

typedef struct {
  PIO pio;
  uint rx_sm;
  uint tx_sm;
} PIO_state;

void generate_crc8_table(void);
uint8_t crc8(const uint8_t *data, size_t len);
bool crc8_check_command(const Command *cmd);
bool crc8_check_data(const Data *data);
void receive_command_payload(PIO_state *pio_state, Payload *payload);
void receive_command(PIO_state *pio_state, Command *cmd);
void receive_data(PIO_state *pio_state, Data *data);
void process_incoming_commands(PIO_state *pio_state);

#endif /* _V9KPROTOCOLS_H_ */