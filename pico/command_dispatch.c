#include "../common/protocols.h"
#include "../common/dos_device_payloads.h"
#include "command_dispatch.h"
#include "sd_block_device.h"

bool dispatchCommand(SDState *sdState, PIO_state *pio_state, Payload *payload) {
    switch (payload->cmd->protocol) {
        case SD_BLOCK_DEVICE:
            return executeSDBlockCommand(sdState, pio_state, payload);
            break;
        default:
            return PROTOCOL_UNKNOWN;
    }
}

// Command Dispatch
bool executeSDBlockCommand(SDState *sdState, PIO_state *pio_state, Payload *payload) {

    switch (payload->cmd->command) {
        case DEVICE_INIT:
            return initSDCard(sdState, pio_state, payload);
            break;
        case MEDIA_CHECK:
            return mediaCheck(sdState, pio_state, payload);
            break;
        case BUILD_BPB:
            return buildBpb(sdState, pio_state, payload);
            break;
        case IOCTL_INPUT:
            return victor9kDriveInfo(sdState, pio_state, payload);
            break;
        case READ_BLOCK:
            return sdRead(sdState, pio_state, payload);
            break;
        case WRITE_NO_VERIFY:
            return sdWrite(sdState, pio_state, payload);
            break;
         case WRITE_VERIFY:
            return sdWrite(sdState, pio_state, payload);
            break;
        default:
            return send_error_response(sdState, pio_state, payload);
            break;
    }
}
 