#define __GNU_VISIBLE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "receive_fifo.pio.h"
#include "transmit_fifo.pio.h"
#include "../sdio-fatfs/include/FatFsSd_C.h"
#include "../sdio-fatfs/src/include/f_util.h"
#include "../sdio-fatfs/src/ff15/source/ff.h"

#include "../../common/protocols.h"
#include "pico_common.h"
#include "sd_block_device.h"

/* SDIO Interface */
static sd_sdio_if_t sdio_if = {
    /*
    Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
    The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
        CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
        As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
            which is -2 in mod32 arithmetic, so:
        CLK_gpio = D0_gpio -2.
        D1_gpio = D0_gpio + 1;
        D2_gpio = D0_gpio + 2;
        D3_gpio = D0_gpio + 3;
    */
    .CMD_gpio = 1,
    .D0_gpio = 2,
    .baud_rate = 15 * 1000 * 1000  // 15 MHz
};

/* Hardware Configuration of the SD Card socket "object" */
static sd_card_t sd_card_sdio = {
    .type = SD_IF_SDIO,
    .sdio_if_p = &sdio_if
};

// /* SPI Interface */
// static spi_t spi = {  
//     .hw_inst = spi0,  // RP2040 SPI component
//     .sck_gpio = 2,    // GPIO number (not Pico pin number)
//     .mosi_gpio = 3,
//     .miso_gpio = 4,
//     .baud_rate = 12 * 1000 * 1000   // Actual frequency: 10416666.
// };

// /* SPI Interface */
// static sd_spi_if_t spi_if = {
//     .spi = &spi,  // Pointer to the SPI driving this card
//     .ss_gpio = 5      // The SPI slave select GPIO for this SD card
// };

/* Configuration of the SD Card socket object */
// static sd_card_t sd_card = {   
//     .type = SD_IF_SDIO,
//     .spi_if_p = &sd_card_sdio  // Pointer to the SPI interface driving this card
// };


/* Callbacks used by the library: */
size_t sd_get_num() { 
    return 1; 
}

sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num)
        return &sd_card_sdio;
    else
        return NULL;
}

// Function to check if a file matches the given pattern
int matchesPattern(const char *filename) {
    if (strlen(filename) < 8) return 0; // Minimum length for valid filenames (e.g., 0_pc.img)

    // Check for "X_pc" or "X_v9k" pattern where X is a digit
    if ((strcasestr(filename, "_pc") != 0 || strcasestr(filename, "_v9k") != 0) &&
        strcasestr(filename, ".img") != 0) {
        return 1;
    }
    return 0;
}

SDState* initializeSDState(const char *directory) {
    printf("Initializaing SD Card...\n");
    SDState *sdState = malloc(sizeof(SDState));
    if (!sdState) {
        perror("Failed to allocate SDState");
        return NULL;
    }

    FATFS fs;
    FRESULT fr = f_mount(&fs, "", 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char* const filename = "output.log";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("fprintf failed\n");
    }

    printf("Mounted SD card\n");
    DIR dir;
    struct dirent *entry;
    sdState->fileCount = 0;

    fr = f_opendir(&dir, directory);
    if (FR_OK != fr) {
        perror("opendir");
        free(sdState);
        return NULL;
    }

    FILINFO fno;
    while (FR_OK == f_readdir(&dir, &fno) ) {
        printf("directory entry: %s\n", fno.fname);
        if (fno.fname[0] == 0) {
            printf("No more files\n");
            break;
        }
        if (matchesPattern(fno.fname)) {
            printf("Found matching file: %d %s\n", sdState->fileCount, fno.fname);
            strncpy(sdState->imgFiles[sdState->fileCount], fno.fname, FILENAME_MAX_LENGTH - 1);
            sdState->imgFiles[sdState->fileCount][FILENAME_MAX_LENGTH - 1] = '\0';
            sdState->fileCount++;
            if (sdState->fileCount >= MAX_IMG_FILES) break;
        }
    }
    f_closedir(&dir);
    return sdState;
}

void freeSDState(SDState *sdState) {
    free(sdState);
}

bool initSDCard(SDState *sdState, PIO_state *pio_state, Payload *payload) {
    const char *directory = "/";
    sdState = initializeSDState(directory);

    if (sdState == NULL) {
        printf("Failed to initialize SD state\n");
        return 1;
    }

    printf("Found %d matching files:\n", sdState->fileCount);
    for (int i = 0; i < sdState->fileCount; i++) {
        printf("%s\n", sdState->imgFiles[i]);
    }

}

bool mediaCheck(SDState *sdState, PIO_state *pio_state, Payload *payload) {

}

bool buildBpb(SDState *sdState, PIO_state *pio_state, Payload *payload) {

}

bool victor9kDriveInfo(SDState *sdState, PIO_state *pio_state, Payload *payload) {

}

bool sdRead(SDState *sdState, PIO_state *pio_state, Payload *payload) {
    //todo fix so driveNumber is pulled from payload
    int driveNumber = 0;
    char *imgFilePath = sdState->imgFiles[driveNumber];

    FIL *imgFile;
    FRESULT result = f_open(imgFile, imgFilePath, FA_READ | FA_WRITE | FA_OPEN_EXISTING);
    if (imgFile == NULL) {
        perror("Failed to open image file");
        return -1;
    }

    // Calculate the offset in the .img file
    //tod fix so startSector is pulled from payload
    int startSector = 0;
    long offset = startSector * SECTOR_SIZE;

    // Move to the calculated offset
    if (FR_OK != f_lseek(imgFile, offset)) {
        perror("Failed to seek to offset");
        f_close(imgFile);
        return -1;
    }

    // Calculate the number of bytes to read
    //todo fix so sectorCount is pulled from payload
    int sectorCount = 1;
    size_t bytesToRead = sectorCount * SECTOR_SIZE;

    // Read the data into the buffer
    //todo fix so buffer size is determined form payload
    char *buffer = (char *)malloc(bytesToRead);
    size_t *bytesRead;
    result = f_read(imgFile, buffer, bytesToRead, bytesRead);
    if (FR_OK != result) {
        perror("Failed to read the expected number of bytes");
        f_close(imgFile);
        return -1;
    }

    f_close(imgFile);
    return 0;
}

bool sdWrite(SDState *sdState, PIO_state *pio_state, Payload *payload) {

}

bool send_error_response(SDState *sdState, PIO_state *pio_state, Payload *payload) {
    return -1;
}