#include <stdint.h>
#include <stdbool.h>

#include "protocols.h"

void generate_crc8_table(void);
uint8_t crc8(const uint8_t *data, size_t len);
bool crc8_check_command(const VictorCommand *cmd);
bool crc8_check_data(const Data *data);