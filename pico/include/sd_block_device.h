#ifndef SD_BLOCK_DEVICE_H
#define SD_BLOCK_DEVICE_H

#include "../../common/protocols.h"
#include "pico_common.h"

#define MAX_IMG_FILES 9
#define FILENAME_MAX_LENGTH 260
#define SECTOR_SIZE 512

typedef struct {
    char imgFiles[MAX_IMG_FILES][FILENAME_MAX_LENGTH];
    int fileCount;
} SDState;

SDState* initializeSDState(const char *directory);
bool initSDCard(SDState *sdState, PIO_state *pio_state, Payload *payload);
bool mediaCheck(SDState *sdState, PIO_state *pio_state, Payload *payload);
bool buildBpb(SDState *sdState, PIO_state *pio_state, Payload *payload);
bool victor9kDriveInfo(SDState *sdState, PIO_state *pio_state, Payload *payload);
bool sdRead(SDState *sdState, PIO_state *pio_state, Payload *payload);
bool sdWrite(SDState *sdState, PIO_state *pio_state, Payload *payload);
bool send_error_response(SDState *sdState, PIO_state *pio_state, Payload *payload);

#endif // SD_BLOCK_DEVICE_H