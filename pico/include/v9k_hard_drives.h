#include "../../common/protocols.h"
#include "pico_common.h"
#include "../sdio-fatfs/src/ff15/source/ff.h"


// Function Prototypes
int read_drive_label(FIL *disk_image, V9kDriveLabel *drive_label);
int parse_var_lists(V9kDriveLabel *drive_label, V9kMediaList *available_media_list, V9kMediaList *working_media_list, V9kVolumeList *volume_list);
int read_virtual_volume_label(FIL *disk_image, uint32_t volume_address, V9kVirtualVolumeLabel *vvl);
void free_media_list(V9kMediaList *media_list);
void free_volume_list(V9kVolumeList *volume_list);
uint16_t calculate_sectors_per_track(uint32_t total_sectors, uint16_t num_cylinders, uint8_t num_heads);