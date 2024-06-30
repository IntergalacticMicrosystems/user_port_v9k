#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DEVICE_INIT,          // 0x00 Initialize
    MEDIA_CHECK,          // 0x01 Media Check
    BUILD_BPB,            // 0x02 Build BPB
    IOCTL_INPUT,          // 0x03 Ioctl In
    READ_BLOCK,           // 0x04 Input (Read)
    NON_DESTRUCTIVE_READ, // 0x05 Non-destructive Read
    INPUT_STATUS,         // 0x06 Input Status
    INPUT_FLUSH,          // 0x07 Input Flush
    WRITE_NO_VERIFY,      // 0x08 Output (Write)
    WRITE_VERIFY,         // 0x09 Output with verify
    OUTPUT_STATUS,        // 0x0A Output Status
    OUTPUT_FLUSH,         // 0x0B Output Flush
    IOTCTL_OUTPUT,        // 0x0C Ioctl Out
    OPEN,                 // 0x0D Device Open
    CLOSE,                // 0x0E Device Close
    REMOVABLE_MEDIA,      // 0x0F Removable Media
    OUTPUT_TILL_BUSY,     // 0x10 Output till busy
    NULL_11,                 // 0x11 Unused
    NULL_12,                 // 0x12 Unused
    GENERIC_IOCTL,        // 0x13 Generic Ioctl
    NULL_14,                 // 0x14 Unused
    NULL_15,                 // 0x15 Unused
    NULL_16,                 // 0x16 Unused
    GET_LOGICAL_DEVICE,   // 0x17 Get Logical Device
    SET_LOGICAL_DEVICE,   // 0x18 Set Logical Device
    IOTCTL_QUERY          // 0x19 Ioctl Query
} DOSDeviceCommand;

typedef struct {
    uint8_t length;       /*  length of the header, in   uint8_ts  */
    uint8_t unit;         /*  physical unit number requested  */
    uint8_t command;      /*  device driver command code      */
    uint16_t status;      /*  status returned by the driver   */
    uint8_t reserved[8];  /*  reserved (unused) uint8_ts      */
} RequestHeaderPayload;

typedef struct {              /* BIOS Parameter block structure...   */
    uint16_t bytes_per_sector;       /*  sector size, in   uint8_ts     */
     uint8_t sectors_per_cluster;    /*  allocation unit size      */
    uint16_t reserved_sectors;  /*  number of reserved (boot) sectors  */
     uint8_t num_fats;          /*  number of FATs on disk    */
    uint16_t root_entry_count;    /*  root directory size, in files   */
    uint16_t total_sectors;     /*  device size, in sectors      */
     uint8_t media_descriptor;   /*  media descriptor code from the BIOS   */
    uint16_t sectors_per_fat;       /*  number of sectors per FAT    */
    uint16_t sectors_per_track;        /*  track size, in sectors    */
    uint16_t num_heads;        /*  number of heads        */
    uint32_t hidden_sectors;     /*  offset of this hard disk partition */
} BPB;

typedef struct {
    uint8_t num_units;        /*  number of units supported by driver   */
    BPB bios_param_block;      /*  actual BIOS Parameter Block   */
    uint8_t drive_number;     /*  first available drive number */
} InitPayload;

typedef struct {
    uint8_t media_descriptor;  /*  media descriptor uint8_t from BIOS */
    uint8_t return_code;       /*  media changed status return code   */
} MediaCheckPayload;

typedef struct {
    uint8_t media_descriptor;  /*  media descriptor uint8_t from BIOS */
    //todo: move firstSector to command Payload
    char *first_sector[512];   /*  the frist sector handed to us from DOS*/
    BPB bios_param_block;       /*  actual BIOS Parameter Block   */
} BuildBpbPayload;

typedef struct { 
    int8_t media_descriptor;  /*  Media Descriptor    */
    uint16_t sector_count;    /*  Byte/Sector Count   */
    uint16_t start_sector;    /*  Starting Sector No. */
    uint8_t volume_id;        /*  Pointer to volume id */ 
} ReadPayload;

typedef struct {  
    int8_t media_descriptor;  /*  Media Descriptor    */
    uint16_t sector_count;    /*  Byte/Sector Count   */
    uint16_t start_sector;    /*  Starting Sector No. */
    uint8_t volume_id;        /*  Pointer to volume id */ 
} WritePayload;