void handle_sd_command(const uint8_t *command, size_t command_size);
void sd_initialize(void);
int sd_read_block(uint32_t block, uint8_t *buffer, uint16_t count);
int sd_write_block(uint32_t block, const uint8_t *buffer, uint16_t count);
