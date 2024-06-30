#ifndef MOCK_SD_CARD_H
#define MOCK_SD_CARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Mock function declarations
bool mock_sd_initialize(uint8_t unit, uint8_t partno);
int mock_sd_read(uint16_t unit, uint32_t block, uint8_t *buffer, uint16_t count);
int mock_sd_write(uint16_t unit, uint32_t block, const uint8_t *buffer, uint16_t count);

#endif // MOCK_SD_CARD_H

