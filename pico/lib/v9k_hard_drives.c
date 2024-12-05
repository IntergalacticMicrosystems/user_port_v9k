#define __GNU_VISIBLE 1
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "../../common/protocols.h"
#include "../../common/dos_device_payloads.h"
#include "../../common/crc8.h"
#include "pico_common.h"
#include "sd_block_device.h"

// Function to correct endianness of a 16-bit value
uint16_t fix_endianness(uint16_t value) {
    return (value >> 8) | (value << 8);
}

// Function to Read the V9K Drive Label from the Disk Image
int read_drive_label(FIL *disk_image, V9kDriveLabel *drive_label) {
    FRESULT res;
    UINT bytes_read;

    // Read drive label from sector 0 of disk image
    res = f_rewind(disk_image);
    if (res != FR_OK) {
        printf("Error rewinding disk image\n");
        return -1;
    }
    res = f_read(disk_image, drive_label, sizeof(V9kDriveLabel), &bytes_read);
    if (bytes_read != sizeof(V9kDriveLabel)) {
        printf("Error reading drive label bytes_read: %d, sizeof(V9kDriveLabel): %d\n", bytes_read, sizeof(V9kDriveLabel));
        return -1;
    }

    printf("Size of V9kDriveLabel: %zu\n", sizeof(V9kDriveLabel));
    printf("Offsets: label_type: %zu, device_id: %zu, serial_number: %zu num_cylinders: %zu\n",
       offsetof(V9kDriveLabel, label_type),
       offsetof(V9kDriveLabel, device_id),
       offsetof(V9kDriveLabel, serial_number),
       offsetof(V9kDriveLabel, num_cylinders));
    
    // Correct the endianness of a few fields. Some of the values were stored in big-endian 
    // format, so we need to fix them. The victor docs defined these values like: 
    // # Cylinders BYTE(Hi) 00Hex
    //             BYTE(Lo) E6Hex (=230)
    // the other utf16_t / utf32_t values were defined as WORDs and read as little-endian
    drive_label->num_cylinders = fix_endianness(drive_label->num_cylinders);
    drive_label->first_rw_reduced = fix_endianness(drive_label->first_rw_reduced);
    drive_label->first_write_precomp = fix_endianness(drive_label->first_write_precomp);

    return 0;
}

// Function to Parse the V9K Variable Lists from the Drive Label
int parse_media_list(FIL *disk_image, MediaList *media_list) {
    FRESULT res;
    UINT bytes_read;

    // Read Available Media List
    res = f_read(disk_image, &media_list->num_regions, sizeof(uint8_t), &bytes_read);
    if (bytes_read != 1) {
        printf("Error reading Available Media List count");
        return -1;
    }

    media_list->regions = malloc(media_list->num_regions * sizeof(Region));
    res = f_read(disk_image, media_list->regions, media_list->num_regions * sizeof(Region), &bytes_read);
    if (bytes_read != media_list->num_regions * sizeof(Region)) {
        printf("Error reading Available Media List regions");
        free(media_list->regions);
        return -1;
    }

    return 0;
}

// Function to Parse the Virtual Volume List from the Drive Label
int parse_volume_list(FIL *disk_image, VolumeList *volume_list) {
    FRESULT res;
    UINT bytes_read;

    // Read Available Media List
    res = f_read(disk_image, &volume_list->num_volumes, sizeof(uint8_t), &bytes_read);
    if (bytes_read != 1) {
        printf("Error reading Virtual Volume List count");
        return -1;
    }

    res = f_read(disk_image, volume_list->volume_addresses, volume_list->num_volumes * sizeof(uint32_t), &bytes_read);
    if (bytes_read != volume_list->num_volumes * sizeof(uint32_t)) {
        printf("Error reading Virtual Volume List addresses");
        return -1;
    }

    return 0;
}

// Function to Read a V9K Virtual Volume Label from the Disk Image
int read_virtual_volume_label(FIL *disk_image, uint32_t volume_address, VirtualVolumeLabel *vir_volume_label) {
    FRESULT res;
    UINT bytes_read;   

    uint64_t offset = (uint64_t)volume_address * SECTOR_SIZE; 

    res = f_lseek(disk_image, offset);
    if (res != FR_OK) {
        printf("Error seeking to virtual volume label");
        return -1;
    }

    res = f_read(disk_image, vir_volume_label, sizeof(VirtualVolumeLabel), &bytes_read); 
    if (bytes_read != sizeof(VirtualVolumeLabel)) {
        printf("Error reading virtual volume label");
        return -1;
    }

    return 0;
}

// Function to V9K Free Media List Memory
void free_media_list(V9kMediaList *media_list) {
    if (media_list->regions) {
        free(media_list->regions);
        media_list->regions = NULL;
    }
}

// Function to V9K Free Volume List Memory
void free_volume_list(V9kVolumeList *volume_list) {
    if (volume_list->volume_addresses) {
        free(volume_list->volume_addresses);
        volume_list->volume_addresses = NULL;
    }
}

uint16_t calculate_sectors_per_track(uint32_t total_sectors, uint16_t num_cylinders, uint8_t num_heads) {
    if (num_cylinders == 0 || num_heads == 0) {
        printf("Error: Division by zero in sectors per track calculation\n");
        return 0;
    }
    return total_sectors / (num_cylinders * num_heads);
}