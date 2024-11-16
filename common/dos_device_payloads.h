#ifndef _DOS_DEVICE_PAYLOADS_H_
#define _DOS_DEVICE_PAYLOADS_H_

#include <stdint.h>
#include <stdbool.h>

#include "protocols.h"

#pragma pack(push, 1)
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

// Structure for a partition table entry in the MBR
typedef struct {
    uint8_t  boot_indicator;    // Boot indicator (0x80 = bootable)
    uint8_t  start_chs[3];      // Starting CHS address
    uint8_t  partition_type;    // Partition type
    uint8_t  end_chs[3];        // Ending CHS address
    uint32_t start_lba;         // Starting LBA address
    uint32_t size_in_sectors;   // Size of the partition in sectors
} PartitionEntry;

// Structure for the MBR
typedef struct {
    uint8_t       bootloader_code[446];   // Bootloader code
    PartitionEntry partition_table[4];    // Partition table entries
    uint16_t      boot_signature;         // Boot signature (should be 0xAA55)
} MBR;

typedef struct {                     /* Victor BIOS Parameter block structure */
    uint16_t bytes_per_sector;       /*  sector size, in   uint8_ts     */
     uint8_t sectors_per_cluster;    /*  allocation unit size      */
    uint16_t reserved_sectors;       /*  number of reserved (boot) sectors  */
     uint8_t num_fats;               /*  number of FATs on disk    */
    uint16_t root_entry_count;       /*  root directory size, in files   */
    uint16_t total_sectors;          /*  device size, in sectors      */
     uint8_t media_descriptor;       /*  media descriptor code from the BIOS   */
    uint16_t sectors_per_fat;        /*  number of sectors per FAT    */
    uint16_t sectors_per_track;      /*  track size, in sectors    */
    uint16_t num_heads;              /*  number of heads        */
    uint32_t hidden_sectors;         /*  offset of this hard disk partition */
    uint32_t partition_start_lba;   /*  offset of the partition inside disk */
} VictorBPB;

// Structure for the BIOS Parameter Block (FAT12/FAT16)
typedef struct {
    uint8_t  jmp_boot[3];             // Jump instruction
    uint8_t  oem_name[8];             // OEM Name
    uint16_t bytes_per_sector;        // Bytes per sector
    uint8_t  sectors_per_cluster;     // Sectors per cluster
    uint16_t reserved_sector_count;   // Reserved sectors count
    uint8_t  num_fats;                // Number of FATs
    uint16_t root_entry_count;        // Root entries count
    uint16_t total_sectors_16;        // Total sectors (if zero, use total_sectors_32)
    uint8_t  media_type;              // Media type
    uint16_t fat_size_16;             // FAT size in sectors
    uint16_t sectors_per_track;       // Sectors per track
    uint16_t num_heads;               // Number of heads
    uint32_t hidden_sectors;          // Hidden sectors
    uint32_t total_sectors_32;        // Total sectors (if total_sectors_16 is zero)
    // Extended BIOS Parameter Block
    uint8_t  drive_number;            // Drive number
    uint8_t  reserved1;               // Reserved
    uint8_t  boot_signature;          // Boot signature (0x29)
    uint32_t volume_id;               // Volume ID
    uint8_t  volume_label[11];        // Volume Label
    uint8_t  fs_type[8];              // File system type (e.g., "FAT12   ")
    uint8_t  boot_code[448];          // Boot code
    uint16_t end_of_sector_marker;    // End of sector marker (0xAA55)
} BPB_FAT12;

typedef struct {              /* FAT16 BIOS Parameter block structure */
    uint16_t bytes_per_sector;       /* sector size, in bytes */
    uint8_t sectors_per_cluster;     /* allocation unit size */
    uint16_t reserved_sectors;       /* number of reserved (boot) sectors */
    uint8_t num_fats;                /* number of FATs */
    uint16_t root_entry_count;       /* root directory entries */
    uint16_t total_sectors_small;    /* total sectors (if < 65536) */
    uint8_t media_descriptor;        /* media descriptor */
    uint16_t sectors_per_fat;        /* sectors per FAT */
    uint16_t sectors_per_track;      /* sectors per track */
    uint16_t num_heads;              /* number of heads */
    uint32_t hidden_sectors;         /* hidden sectors before this partition */
    uint32_t total_sectors_large;    /* total sectors (if >= 65536) */
} FAT16BPB;

// V9K HDvRegion Descriptor Structure
typedef struct {
    uint32_t region_pa;     // Physical Address
    uint32_t region_size;   // Block Count
} V9kRegionDescriptor;

// V9K HD Media List Structure
typedef struct {
    uint8_t region_count;
    V9kRegionDescriptor *regions; // Dynamic array of region descriptors
} V9kMediaList;

// V9K HD Volume List Structure
typedef struct {
    uint8_t volume_count;
    uint32_t *volume_addresses; // Dynamic array of volume addresses
} V9kVolumeList;

// V9K HD Drive Label Structure
typedef struct {
    uint16_t label_type;        // Label Type
    uint16_t device_id;         // Device ID
    uint8_t serial_number[16];  // Serial Number
    uint16_t sec_size;          // Sector Size

    // IPL Vector
    uint32_t ipl_da;            // Disk Address
    uint16_t ipl_la;            // Load Address
    uint16_t ipl_llength;       // Load Length
    uint32_t ipl_entry;         // Code Entry PTR

    uint16_t p_boot_vol;        // Primary Boot Volume
    uint8_t ctrl_parms[16];     // Control Parameters

    // Variable Lists (Available Media List, Working Media List, Virtual Volume List)
    uint8_t var_lists[972];
} V9kDriveLabel;

// V9K HD Virtual Volume Label Structure
typedef struct {
    uint16_t label_type;         // Label Type
    uint8_t volume_name[16];     // Volume Name

    // IPL Vector
    uint32_t ipl_da;             // Disk Address
    uint16_t ipl_la;             // Load Address
    uint16_t ipl_llength;        // Load Length
    uint32_t ipl_entry;          // Code Entry PTR

    uint32_t capacity;           // Volume Capacity
    uint32_t data_start;         // Data Start
    uint16_t block_size;         // Host Block Size
    uint16_t au;                 // Allocation Unit
    uint16_t dirent;             // Number of Directory Entries
    uint8_t reserved[16];        // Reserved
    uint8_t assign_count;        // Assignment Count
    struct {
        uint16_t device_unit;    // Device Unit
        uint16_t volume_index;   // Volume Index
    } assignments[16];           // Assignments (up to 16)
    uint8_t extra[387];          // Remaining bytes to fill sector
} V9kVirtualVolumeLabel;

typedef struct {
    uint16_t num_cylinders;        /* Number of Cylinders */
    uint8_t num_heads;             /* Number of Heads */
    uint16_t reduced_current_cyl;  /* First Reduced-Current Cylinder */
    uint16_t write_precomp_cyl;    /* First Write Precompensation Cylinder */
    uint8_t ecc_burst_length;      /* ECC Data Burst Length */
    uint8_t options;               /* Options */
    uint8_t interleave;            /* Interleave */
    uint8_t spares[6];             /* Spares (6 bytes) */
} V9KHardDriveControlParameters;

typedef struct {
    uint8_t num_units;    /*  number of disk images found   */
    VictorBPB bpb_array[MAX_IMG_FILES];   /*  array of BIOS Parameter Blocks   */
} InitPayload;

typedef struct {
    uint8_t media_descriptor;  /*  media descriptor uint8_t from BIOS */
    uint8_t return_code;       /*  media changed status return code   */
} MediaCheckPayload;

typedef struct {
    uint8_t media_descriptor;  /*  media descriptor uint8_t from BIOS */
    //todo: move firstSector to command Payload
    char *first_sector[512];   /*  the frist sector handed to us from DOS*/
    VictorBPB bios_param_block;       /*  actual BIOS Parameter Block   */
} BuildBpbPayload;

typedef struct { 
    int8_t media_descriptor;  /*  Media Descriptor    */
    uint16_t sector_count;    /*  Byte/Sector Count   */
    uint16_t start_sector;    /*  Starting Sector No. */
    uint8_t drive_number;     /*  Drive Image Number  */ 
} ReadParams;

typedef struct {  
    int8_t media_descriptor;  /*  Media Descriptor    */
    uint16_t sector_count;    /*  Byte/Sector Count   */
    uint16_t start_sector;    /*  Starting Sector No. */
    uint8_t drive_number;     /*  Drive Image Number  */ 
} WriteParams;


#pragma pack(pop)

#endif /* _DOS_DEVICE_PAYLOADS_H_ */
