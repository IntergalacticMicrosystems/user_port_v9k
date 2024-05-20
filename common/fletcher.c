#include <stdint.h>

void fletcher16_byte(uint16_t *sum1, uint16_t *sum2, uint8_t value) {
    *sum1 = (*sum1 + value) % 255;
    *sum2 = (*sum2 + *sum1) % 255;
}

uint16_t fletcher16_finalize(uint16_t sum1, uint16_t sum2) {
    return (sum2 << 8) | sum1;
}
