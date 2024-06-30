#include <stdint.h>
#include <stdbool.h>

#include "protocols.h"

void generateCrc8Table(void);
uint8_t crc8(const uint8_t *data, size_t len);
void createCommandCrc8(VictorCommand *cmd);
bool isValidCommandCrc8(const VictorCommand *cmd);
bool isValidDataCrc8(const Data *data);