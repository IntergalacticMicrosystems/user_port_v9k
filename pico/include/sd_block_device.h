#ifndef SD_BLOCK_DEVICE_H
#define SD_BLOCK_DEVICE_H

#include "../../common/protocols.h"
#include "../../common/dos_device_payloads.h"
#include "pico_common.h"
#include "../sdio-fatfs/src/ff15/source/ff.h"

typedef struct {
    FIL *img_file;
    uint32_t start_lba;
} DriveImage;

typedef struct {
    char file_names[MAX_IMG_FILES][FILENAME_MAX_LENGTH];
    int fileCount;
    FATFS *fs;
    DriveImage *images[MAX_IMG_FILES];
    FIL *debug_log;
} SDState;

void print_debug_bpb(VictorBPB *bpb);
SDState* initialize_sd_state(const char *directory);
Payload* init_sd_card(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* media_check(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* build_bpb(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* victor9k_drive_info(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* sd_read(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* sd_write(SDState *sdState, PIO_state *pio_state, Payload *payload);
Payload* create_error_response(SDState *sdState, PIO_state *pio_state, Payload *input);

#endif // SD_BLOCK_DEVICE_H