#include <stdlib.h>
#include <string.h>

#include "../../common/protocols.h"
#include "../../common/dos_device_payloads.h"
#include "command_dispatch.h"
#include "sd_block_device.h"

Payload* dispatch_command(SDState *sdState, PIO_state *pio_state, Payload *payload) {
    switch (payload->protocol) {
        case SD_BLOCK_DEVICE:
            return execute_sd_block_command(sdState, pio_state, payload);
            break;
        default:
            payload->status = INVALID_PROTOCOL;
            return create_error_response(sdState, pio_state, payload);
            break;
    }
}

// Command Dispatch
Payload* execute_sd_block_command(SDState *sdState, PIO_state *pio_state, Payload *payload) {
    Payload* response = (Payload*)malloc(sizeof(Payload));
    memset (response, 0, sizeof(Payload));

    switch (payload->command) {
        case DEVICE_INIT:
            response = init_sd_card(sdState, pio_state, payload);
            break;
        case MEDIA_CHECK:
            response = media_check(sdState, pio_state, payload);
            break;
        case BUILD_BPB:
            response = build_bpb(sdState, pio_state, payload);
            break;
        case IOCTL_INPUT:
            response = victor9k_drive_info(sdState, pio_state, payload);
            break;
        case READ_BLOCK:
            response = sd_read(sdState, pio_state, payload);
            break;
        case WRITE_NO_VERIFY:
            response = sd_write(sdState, pio_state, payload);
            break;
         case WRITE_VERIFY:
            response = sd_write(sdState, pio_state, payload);
            break;
        default:
            payload->status = INVALID_COMMAND;
            response = create_error_response(sdState, pio_state, payload);
            break;
    }

    return response;
}
 