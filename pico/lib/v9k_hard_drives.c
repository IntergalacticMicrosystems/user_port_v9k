#define __GNU_VISIBLE 1
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../../common/protocols.h"
#include "../../common/dos_device_payloads.h"
#include "../../common/crc8.h"
#include "pico_common.h"
#include "sd_block_device.h"


// Function to Read the V9K Drive Label from the Disk Image
int read_drive_label(FIL *disk_image, V9kDriveLabel *drive_label) {
    FRESULT res;
    UINT bytes_read;

    // Read sector 0 (512 bytes)
    res = f_read(disk_image, drive_label, sizeof(V9kDriveLabel), &bytes_read);
    if (bytes_read != sizeof(V9kDriveLabel)) {
        printf("Error reading drive label bytes_read: %d, sizeof(V9kDriveLabel): %d\n", bytes_read, sizeof(V9kDriveLabel));
        return -1;
    }

    return 0;
}

// Function to Parse the V9K Variable Lists from the Drive Label
int parse_var_lists(V9kDriveLabel *drive_label, V9kMediaList *available_media_list, V9kMediaList *working_media_list, V9kVolumeList *volume_list) {
    uint8_t *ptr = drive_label->var_lists;
    uint8_t *end_ptr = drive_label->var_lists + sizeof(drive_label->var_lists);

    // Parse Available Media List
    if (ptr >= end_ptr) {
        printf("Error parsing Available Media List\n");
        return -1;
    }
    available_media_list->region_count = *ptr++;
    if (available_media_list->region_count > 0) {
        size_t regions_size = available_media_list->region_count * sizeof(V9kRegionDescriptor);
        if (ptr + regions_size > end_ptr) {
            printf("Error parsing Available Media List regions\n");
            return -1;
        }
        available_media_list->regions = malloc(regions_size);
        if (!available_media_list->regions) {
            printf("Error allocating memory for Available Media List regions");
            return -1;
        }
        memcpy(available_media_list->regions, ptr, regions_size);
        ptr += regions_size;
    }

    // Parse Working Media List
    if (ptr >= end_ptr) {
        printf("Error parsing Working Media List\n");
        return -1;
    }
    working_media_list->region_count = *ptr++;
    if (working_media_list->region_count > 0) {
        size_t regions_size = working_media_list->region_count * sizeof(V9kRegionDescriptor);
        if (ptr + regions_size > end_ptr) {
            printf("Error parsing Working Media List regions\n");
            return -1;
        }
        working_media_list->regions = malloc(regions_size);
        if (!working_media_list->regions) {
            printf("Error allocating memory for Working Media List regions");
            return -1;
        }
        memcpy(working_media_list->regions, ptr, regions_size);
        ptr += regions_size;
    }

    // Parse Virtual Volume List
    if (ptr >= end_ptr) {
        printf("Error parsing Virtual Volume List\n");
        return -1;
    }
    volume_list->volume_count = *ptr++;
    if (volume_list->volume_count > 0) {
        size_t volumes_size = volume_list->volume_count * sizeof(uint32_t);
        if (ptr + volumes_size > end_ptr) {
            printf("Error parsing Virtual Volume List addresses\n");
            return -1;
        }
        volume_list->volume_addresses = malloc(volumes_size);
        if (!volume_list->volume_addresses) {
            printf("Error allocating memory for Virtual Volume List addresses");
            return -1;
        }
        memcpy(volume_list->volume_addresses, ptr, volumes_size);
        ptr += volumes_size;
    }

    return 0;
}

// Function to Read a V9K Virtual Volume Label from the Disk Image
int read_virtual_volume_label(FIL *disk_image, uint32_t volume_address, V9kVirtualVolumeLabel *vir_volume_label) {
    FRESULT res;
    UINT bytes_read;   

    uint64_t offset = (uint64_t)volume_address * SECTOR_SIZE; 

    res = f_lseek(disk_image, offset);
    if (res != FR_OK) {
        printf("Error seeking to virtual volume label");
        return -1;
    }

    res = f_read(disk_image, vir_volume_label, sizeof(V9kVirtualVolumeLabel), &bytes_read); 
    if (bytes_read != sizeof(V9kVirtualVolumeLabel)) {
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