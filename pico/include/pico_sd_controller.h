void handle_sd_command(const uint8_t *command, size_t command_size);
void sd_initialize(void);
FRESULT sd_read(uint16_t drive, uint32_t sector, uint8_t *buffer, uint16_t count);
FRESULT sd_write(uint16_t drive, uint32_t sector, uint8_t *buffer, uint16_t count);
