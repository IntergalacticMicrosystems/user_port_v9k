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
#include "../../common/dos_device_payloads.h"
#include "../../common/crc8.h"
#include "pico_common.h"
#include "sd_block_device.h"
#include "v9k_hard_drives.h"

Payload* log_output(SDState *sdState, PIO_state *pio_state, Payload *payload) {

    printf("V9K Log:");
    for (int i = 0; i < payload->data_size; i++) {
        printf("%c", payload->data[i]);
    }

    Payload *response = (Payload*)malloc(sizeof(Payload));
    if (response == NULL) {
        printf("Error: Memory allocation failed for payload\n");
        return NULL;
    }
    memset(response, 0, sizeof(Payload));
    response->protocol = LOG_OUTPUT;
    response->command = LOG_OUTPUT;
    response->params = (uint8_t *)malloc(1);
    if (response->params == NULL) {
        printf("Error: Memory allocation failed for response->params\n");
        free(response);
        return NULL;
    }
    response->params[0] = 0;

    response->data_size = 1;
    response->data = (uint8_t *)malloc(1);
    response->data[0] = 0;
    response->status = STATUS_OK;
    create_command_crc8(response);
    create_data_crc8(response);
    
    return response;

}