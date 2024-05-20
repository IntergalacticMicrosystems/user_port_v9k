bool sd_initialize(uint8_t unit, uint8_t partno);
int sd_read(uint16_t unit, uint32_t block, uint8_t far *buffer, uint16_t count);
int sd_write(uint16_t unit, uint32_t block, const uint8_t far *buffer, uint16_t count);
bool sd_media_check(uint8_t unit);
