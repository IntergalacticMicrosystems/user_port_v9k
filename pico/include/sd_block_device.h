#ifndef SD_BLOCK_DEVICE_H
#define SD_BLOCK_DEVICE_H

#include "../../common/protocols.h"
#include "pico_common.h"

#define MAX_IMG_FILES 16
#define FILENAME_MAX_LENGTH 260
#define SECTOR_SIZE 512

typedef struct {
    char imgFiles[MAX_IMG_FILES][FILENAME_MAX_LENGTH];
    int fileCount;
} SDState;

SDState* initialize_sd_state(const char *directory);
Payload* init_sd_card(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* media_check(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* build_bpb(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* victor9k_drive_info(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* sd_read(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* sd_write(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* create_error_response(SDState *sdState, PIO_state *pio_state, Payload *input);

#endif // SD_BLOCK_DEVICE_H