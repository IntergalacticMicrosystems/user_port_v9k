#ifndef COMMAND_DISPATCH_H
#define COMMAND_DISPATCH_H

#include "pico_common.h"
#include "sd_block_device.h"

typedef enum {
    CMD_INIT,
    CMD_MEDIA_CHECK,
    CMD_BUILD_BPB,
    CMD_IOCTL_V9K_DRIVE_INFO,
    CMD_READ_SD,
    CMD_WRITE_SD
} SDBlockCommand;

Payload* dispatch_command(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* execute_sd_block_command(SDState *sdState, PIO_state *pio_state, Payload *payload);

#endif