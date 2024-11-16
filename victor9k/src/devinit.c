/*
 * Template for writing DOS device drivers in Open Watcom C
 *
 * Copyright (C) 2022, Eduardo Casino (mail@eduardocasino.es)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

/* driver.c - MSDOS device driver functions           */
/*                         */
/* Copyright (C) 1994 by Robert Armstrong          */
/*                         */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or */
/* (at your option) any later version.             */
/*                         */
/* This program is distributed in the hope that it will be useful, but  */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT- */
/* ABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General */
/* Public License for more details.             */
/*                         */
/* You should have received a copy of the GNU General Public License */
/* along with this program; if not, visit the website of the Free */
/* Software Foundation, Inc., www.gnu.org.            */

/* Paul Devine   Sept-2023
 * Combined code from: https://github.com/eduardocasino/dos-device-driver
 * that had a template to build an MS-DOS device driver with openwatcom
 * with code from: https://forum.vcfed.org/index.php?threads/sd-card-to-parallel-port-driver-for-msdos-ver-1-1.42008/
 * which had the code to access the SD card for an IBM-PC over the parallel port
 * I'm adopting that to work with the Victor 9000 / ACT Sirius 1 hardware
 */

#include <dos.h>
#include <stdint.h>
#include <i86.h> 
#include <string.h> 

#include "devinit.h"
#include "template.h"
#include "cprint.h"     /* Console printing direct to hardware */
#include "diskio.h"     /* SD card library header */
#include "v9_communication.h"  /* Victor 9000 communication protocol */
#include "../../common/protocols.h"
#include "../../common/dos_device_payloads.h"
#include "../../common/crc8.h"

static bool validate_far_ptr(void far *ptr, size_t size); // Function to validate a far pointer

#pragma data_seg("_CODE")
//((7*16 + 3*8 +32) * 9 + 1)  (size of VictorBPB) * MAX_DRIVES + 1 for num_units
//my math doesn't match the compiler's math, so I'm going to use the compiler's math
//1711 is observed from running on the Victor9K.
#define MAX_INIT_PAYLOAD_SIZE 1711  

bool debug = true;
static uint8_t portbase;
static uint8_t partition_number = 0;
//
// Place here any variables or constants that should go away after initialization
//
//static char hellomsg[] = "\r\nDOS Device Driver Template in Open Watcom C\r\n$";

extern int8_t num_drives;
//BPB table is an array of near pointers to an array BPB structures
//DOS passes around a far pointer to the table
//these are defined in templace.c to be used in the rest of the code
extern bpb my_bpbs[MAX_IMG_FILES];  // Array of BPB instances
extern bpb near *my_bpb_tbl[MAX_IMG_FILES];   // BPB Table = array of near pointers to BPB structures
extern bpb * __far *my_bpb_tbl_far_ptr;   // Far pointer to the BPB table
extern bool initNeeded;

#ifdef RAMDRIVE
extern MiniDrive myDrive;
#endif


/*   WARNING!!  WARNING!!  WARNING!!  WARNING!!  WARNING!!  WARNING!!   */
/*                         */
/*   All code following this point in the file is discarded after the   */
/* driver initialization.  Make absolutely sure that no routine above   */
/* this line calls any routine below it!!          */
/*                         */
/*   WARNING!!  WARNING!!  WARNING!!  WARNING!!  WARNING!!  WARNING!!   */

/* Driver Initialization */
/*   DOS calls this function immediately after the driver is loaded and */
/* expects it to perform whatever initialization is required.  Since */
/* this function can never be called again, it's customary to discard   */
/* the memory allocated to this routine and any others that are used */
/* only at initialization.  This allows us to economize a little on the */
/* amount of memory used.                 */
/*                         */
/*   This routine's basic function is to initialize the serial port, go */
/* and make contact with the SD card, and then return a table of BPBs to   */
/* DOS.  If we can't communicate with the drive, then the entire driver */
/* is unloaded from memory.                  */
uint16_t deviceInit( void ) {
    struct ALL_REGS registers;
    get_all_registers(&registers);

    fpRequest->r_endaddr = MK_FP(registers.cs, &transient_data);
    struct device_header far *dev_header = MK_FP(registers.cs, 0);

    cdprintf("\nSD pport device driver v0.1 (C) 2023 by Paul Devine\n");
    cdprintf("     based on (C) 2014 by Dan Marks and on TU58 by Robert Armstrong\n");
    cdprintf("     with help from an openwatcom driver by Eduardo Casino\n");

    if (debug) cdprintf("initializing user port\n");
    
    #ifdef RAMDRIVE
    cdprintf("SD:  myDrive = %4x:%4x\n", FP_SEG( &myDrive), FP_OFF( &myDrive));
    #endif

    ResponseStatus status = initialize_user_port();
    if (status != STATUS_OK) {
        cdprintf("Error initializing VIA %u\n",(uint16_t) status);
        return status;
    }
    if (debug) cdprintf("sending startup handshake\n");
    status = send_startup_handshake();
    if (status != STATUS_OK) {
        cdprintf("Error sending startup handshake %u\n", (uint16_t) status);
        return status;
    }

    //address to find passed by DOS in a combo of ES / BX get_all_registers
    if (debug) {
        cdprintf("about to parse bpb, ES: %x BX: %x\n", registers.es, registers.bx);
        cdprintf("about to parse bpb, CS: %x DS: %x\n", registers.cs, registers.ds);
        cdprintf("SD: command: %d r_unit: %d\n", fpRequest->r_command, fpRequest->r_unit);
    }
    char name_buffer[9];  // 8 bytes for the name + 1 byte for the null terminator
    memcpy(name_buffer, (void const *) dev_header->dh_name, 8);
    name_buffer[8] = '\0';

    if (debug) {
        cdprintf("SD: dh_name: %s\n", name_buffer);
        cdprintf("SD: dh_next: %x\n", dev_header->dh_next);
    }

    //initialize the BPB table
    if (debug) cdprintf("SD: initializing BPB table\n");
    for (int i = 0; i < MAX_IMG_FILES; i++) {
        if (debug) cdprintf("SD: my_bpb_tbl[%d] = &my_bpbs[%d] %X\n", i, i, (uint32_t) &my_bpbs[i]);
        my_bpb_tbl[i] = &my_bpbs[i];
    }

    #ifdef RAMDRIVE
    cdprintf("SD: ramdrive allocation succeeded. Total Size: %d KB\n", (sizeof( myDrive.sectors)/1024));
    #endif

    //DOS is overloading a data structure that in normal use stores the BPB, 
    //for init() it stores the string that sits in config.sys
    //hence I'm casting to a char

    char far *bpb_cast_ptr = (char far *)(fpRequest->r_bpb_tbl_ptr);  

    if (debug) cdprintf("gathered bpb_ptr: %x\n", (uint16_t) bpb_cast_ptr);
    /* Parse the options from the CONFIG.SYS file, if any... */
    if (!parse_options((char far *) bpb_cast_ptr)) {
        if (debug) cdprintf("SD: bad options in CONFIG.SYS\n");
        return (S_DONE | S_ERROR | E_UNKNOWN_MEDIA ); 
    }
    if (debug) cdprintf("done parsing bpb_ptr: %x\n", (uint16_t) bpb_cast_ptr);

    /* Try to make contact with the drive... */
    if (debug) cdprintf("SD: initializing drive r_unit: %u\n", (uint16_t) fpRequest->r_unit);
    if (debug) cdprintf("checking CS: %x DS: %x\n", registers.cs, registers.ds);

    if (debug) cdprintf("SD: initializing drive &my_bpb_tbl_far_ptr: %4x:%4x, my_bpb_tbl_far_ptr: %4x:%4x\n", 
       FP_SEG(&my_bpb_tbl_far_ptr), FP_OFF(&my_bpb_tbl_far_ptr), 
       FP_SEG(my_bpb_tbl_far_ptr), FP_OFF(my_bpb_tbl_far_ptr));

    if (debug) cdprintf("SD: initializing drive &my_bpb_tbl[0]: %4x:%4x, my_bpb_tbl[0]: %4x:%4x\n", 
       FP_SEG(&my_bpb_tbl[0]), FP_OFF(&my_bpb_tbl[0]), 
       FP_SEG(my_bpb_tbl[0]), FP_OFF(my_bpb_tbl[0]));

    if (debug) cdprintf("SD: initializing drive &my_bpbs[0]: %4x:%4x\n", 
       FP_SEG(&my_bpbs[0]), FP_OFF(&my_bpbs[0]));
    if (debug) cdprintf("checking CS: %x DS: %x\n", registers.cs, registers.ds);
    Payload initPayload = {0};
    initPayload.protocol = SD_BLOCK_DEVICE;
    initPayload.command = DEVICE_INIT;
    uint8_t init_params[1] = {0};
    initPayload.params_size = sizeof(init_params);
    initPayload.params = &init_params[0];
    uint8_t data[1] = {0};
    uint8_t far *data_ptr = &data[0];
    initPayload.data = data_ptr;
    initPayload.data_size = sizeof(data);
    if (debug) cdprintf("sending data_size: %d\n", initPayload.data_size);
    create_payload_crc8(&initPayload);

    ResponseStatus outcome = send_command_payload(&initPayload);
    if (outcome != STATUS_OK) {
        cdprintf("Error: Failed to send DEVICE_INIT command to SD Block Device %u\n", (uint16_t) outcome);
        return (S_DONE | S_ERROR | E_UNKNOWN_MEDIA );
    }
    
    //getting the response from the SD card
    if (debug) cdprintf("command sent success, starting receive response\n");
    if (debug) cdprintf("checking CS: %x DS: %x\n", registers.cs, registers.ds);
    Payload responsePayload = {0};
    uint8_t response_params[3] = {0};
    responsePayload.params = &response_params[0];
    uint16_t data_size = (sizeof(InitPayload) * MAX_IMG_FILES) + 1;

    if (debug) cdprintf("SD: data_size: %u MAX_INIT_PAYLOAD_SIZE: %u\n", data_size, MAX_INIT_PAYLOAD_SIZE);
    if (data_size != MAX_INIT_PAYLOAD_SIZE) {
        if (debug) cdprintf("SD: data_size != MAX_INIT_PAYLOAD_SIZE, sizeof(InitPayload) has changed\n");
        return (S_DONE | S_ERROR | E_GENERAL_FAILURE);
    }
    uint8_t response_data[MAX_INIT_PAYLOAD_SIZE] = {0};
    uint8_t far *response_data_ptr = &response_data[0];
    responsePayload.data = response_data_ptr;
    
    outcome = receive_response(&responsePayload);
    if (outcome != STATUS_OK) {
        cdprintf("SD Error: Failed to receive response from SD Block Device %u\n", (uint16_t) outcome);
        cdprintf((char *) outcome);
        cdprintf("\n");
        return (S_DONE | S_ERROR | E_UNKNOWN_MEDIA );
    }

    #ifdef RAMDRIVE
    bpb *my_bpb_ptr = &my_bpbs[0]; 
    
    my_bpb_ptr->bpb_nbyte = 512;
    my_bpb_ptr->bpb_nsector = 1;     /* Number of sectors per cluster */
    my_bpb_ptr->bpb_nreserved = 1;  /* Number of reserved sectors */
    my_bpb_ptr->bpb_nfat = 2;              /* Number of FAT copies */
    my_bpb_ptr->bpb_ndirent = 16;   
    my_bpb_ptr->bpb_nsize = 32;
    my_bpb_ptr->bpb_mdesc = 0xF8;
    my_bpb_ptr->bpb_nfsect = 1;             /* Number of sectors per FAT */
   
    
    //set &myDrive.sectors[0] to contain the BpB
    //configure the empty FAT tables for the RAM drive
    myDrive.sectors[1].data[0] = 0xF0;
    myDrive.sectors[1].data[1] = 0xFF;
    myDrive.sectors[1].data[2] = 0xFF;

    myDrive.sectors[2].data[0] = 0xF0;
    myDrive.sectors[2].data[1] = 0xFF;
    myDrive.sectors[2].data[2] = 0xFF;

    uint8_t directory_entry[512] = {
    0x4D, 0x41, 0x43, 0x48,   0x49, 0x4E, 0x45, 0x31,   0x42, 0x20, 0x20, 0x28,   0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x6A, 0x91,   0x29, 0x54, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00 };
    if (debug) cdprintf("checking CS: %x DS: %x\n", registers.cs, registers.ds);
    // Start filling from the 32nd byte
    for (uint16_t i = 32; i < 512;) { // Notice the increment part is empty
        directory_entry[i] = 0x00; // Set the current byte to 0x00
        i++;
        for (uint16_t j = 0; j < 31 && i < 512; j++, i++) { // Also check i < 512 to avoid overflow
            directory_entry[i] = 0x93;
        }
        cdprintf("%d ", i);

    }
    if (debug) cdprintf("checking CS: %x DS: %x\n", registers.cs, registers.ds);
    for (uint16_t i = 0; i < 512; i++) {
        myDrive.sectors[3].data[i] = directory_entry[i];
    }
    #endif

    if (debug) cdprintf("checking CS: %x DS: %x\n", registers.cs, registers.ds);
    //parsing the response
    if (debug) cdprintf("received response, parsing data\n");
    if (debug) cdprintf("SD: sizeof(InitPayload): %u responsePayload.data_size: %u\n", sizeof(InitPayload), responsePayload.data_size);
    if (responsePayload.data_size > sizeof(InitPayload)) {
        if (debug) cdprintf("SD: ERROR: responsePayload.data_size > sizeof(InitPayload)\n");
        return (S_DONE | S_ERROR | E_GENERAL_FAILURE);
    }
    InitPayload *init_details= (InitPayload *) &responsePayload.data[0];
    if (debug) cdprintf("SD: num_drives: %d\n", init_details->num_units);
    if (debug) cdprintf("SD: sizeof(init_details): %u\n", sizeof(*init_details));

    num_drives = init_details->num_units;
    fpRequest->r_nunits = num_drives;         //tell DOS how many drives we're instantiating.
    bpb far *my_bpb_tbl_far_ptr = MK_FP(registers.cs, FP_OFF(&my_bpb_tbl[0]));

    if (!validate_far_ptr(my_bpb_tbl_far_ptr, (sizeof(my_bpb_tbl[0])*num_drives))) {
        if (debug) cdprintf("SD: my_bpb_tbl_far_ptr address\n");
        return (S_DONE | S_ERROR | E_GENERAL_FAILURE);
    }
    fpRequest->r_bpb_tbl_ptr = (bpb * __far *) my_bpb_tbl_far_ptr;  // Pass to DOS the far pointer to our array of bpb poitners
    

    //copy the BPB details to the BPB table
    for (int i = 0; i < num_drives; i++) {
        const VictorBPB *drive = &init_details->bpb_array[i];
        my_bpbs[i].bpb_nbyte = drive->bytes_per_sector;  //copy the BPB details to
        my_bpbs[i].bpb_nsector = drive->sectors_per_cluster;  //the BPB table   
        my_bpbs[i].bpb_nreserved = drive->reserved_sectors;
        my_bpbs[i].bpb_nfat = drive->num_fats;
        my_bpbs[i].bpb_ndirent = drive->root_entry_count;
        my_bpbs[i].bpb_nsize = drive->total_sectors;
        my_bpbs[i].bpb_mdesc = drive->media_descriptor;
        my_bpbs[i].bpb_nfsect = drive->sectors_per_fat;
    }
    
    if (debug) {
        cdprintf("SD: done parsing &my_bpbs[0]: = %4x:%4x\n", FP_SEG(&my_bpbs[0]), FP_OFF(&my_bpbs[0]));
        cdprintf("SD: done parsing &my_bpb_tbl_far_ptr = %4x:%4x\n", FP_SEG(&my_bpb_tbl_far_ptr), FP_OFF(&my_bpb_tbl_far_ptr));
        cdprintf("SD: done parsing my_bpb_tbl_far_ptr = %4x:%4x\n", FP_SEG(my_bpb_tbl_far_ptr), FP_OFF(my_bpb_tbl_far_ptr));
        cdprintf("SD: done parsing registers.cs = %4x:%4x\n", FP_SEG(registers.cs), FP_OFF(registers.ds));
        cdprintf("SD: r_unit: %x\n", fpRequest->r_unit);
    }

    uint32_t bpb_start = calculateLinearAddress(FP_SEG(my_bpb_tbl_far_ptr) , FP_OFF(my_bpb_tbl_far_ptr));

    if (debug) {
        cdprintf("SD: my_bpb = %4x:%4x  %5X\n", FP_SEG(&my_bpbs[0]), FP_OFF(&my_bpbs[0]), bpb_start);
        writeToDriveLog("SD: my_bpb_tbl_far_ptr = %4x:%4x  %5X\n", FP_SEG(my_bpb_tbl_far_ptr), FP_OFF(my_bpb_tbl_far_ptr), bpb_start);
        cdprintf("SD: initialized on DOS drive %c r_firstunit: %d r_nunits: %d\n",(
            fpRequest->r_firstunit + 'A'), fpRequest->r_firstunit, fpRequest->r_nunits);
    
    }

    // /* All is well.  Tell DOS how many units and the BPBs... */
    uint8_t i;
    for (i=0; i < num_drives; i++) {
        if (debug) {cdprintf("SD:  my_drives: %d drive %c\n", i, (fpRequest->r_firstunit + i + 'A'));}
    }
    initNeeded = false;


    if (debug) {   
      cdprintf("SD: BPB data:\n");
      for (int i = 0; i < num_drives; i++) {
        cdprintf("Drive %c %d\n", (fpRequest->r_firstunit + i + 'A'), i);
        cdprintf("&my_bpb_tbl_far_ptr = %4x:%4x\n", FP_SEG(&my_bpb_tbl_far_ptr), FP_OFF(&my_bpb_tbl_far_ptr));
        cdprintf("my_bpb_tbl_far_ptr= %4x:%4x\n", FP_SEG(my_bpb_tbl_far_ptr), FP_OFF(my_bpb_tbl_far_ptr));
        cdprintf("&my_bpb_tbl[%d] FP_SEG = %4x:%4x\n", i, FP_SEG(&my_bpb_tbl[i]), FP_OFF(&my_bpb_tbl[i]));
        cdprintf("my_bpb_tbl[%d] = %4x:%4x\n", i, FP_SEG(my_bpb_tbl[i]), FP_OFF(my_bpb_tbl[i]));
        cdprintf("&my_bpbs[%d] = %4x:%4x\n", i, FP_SEG(&my_bpbs[i]), FP_OFF(&my_bpbs[i]));
        cdprintf("Bytes per Sector: %d\n", my_bpbs[i].bpb_nbyte);
        cdprintf("Sectors per Allocation Unit: %d\n", (uint16_t) my_bpbs[i].bpb_nsector);
        cdprintf("# Reserved Sectors: %d\n", my_bpbs[i].bpb_nreserved);
        cdprintf("# FATs: %d\n", (uint16_t) my_bpbs[i].bpb_nfat);
        cdprintf("# Root Directory entries: %d  ", my_bpbs[i].bpb_ndirent);
        cdprintf("Size in sectors: %u\n", my_bpbs[i].bpb_nsize);
        cdprintf("MEDIA Descriptor Byte: %x\n", (uint16_t) my_bpbs[i].bpb_mdesc);
        cdprintf("FAT size in sectors: %u\n", (uint16_t) my_bpbs[i].bpb_nfsect);
      }
      cdprintf("SD: &transient_data = %4x:%4x\n", FP_SEG(&transient_data), FP_OFF(&transient_data));
      cdprintf("SD: fpRequest->r_endaddr = %4x:%4x\n", FP_SEG(fpRequest->r_endaddr), FP_OFF(fpRequest->r_endaddr));
    }
    if (debug) cdprintf("done parsing, CS: %x DS: %x\n", registers.cs, registers.ds);
    if (debug) cdprintf("returing SD_DONE from init()\n");
  return S_DONE;    
}

/* iseol - return TRUE if ch is any end of line character */
bool iseol (char ch) {
      return ch=='\0' || ch=='\r' || ch=='\n';  
}

/* spanwhite - skip any white space characters in the string */
char far *spanwhite (char far *p) {
      while (*p==' ' || *p=='\t') ++p;  return p;  
}

/* option_value */
/*   This routine will parse the "=nnn" part of an option.  It should   */
/* be called with a text pointer to what we expect to be the '=' char-  */
/* acter.  If all is well, it will return the binary value of the arg-  */
/* ument and a pointer to the first non-numeric character.  If there is */
/* a syntax error, then it will return NULL.          */
char far *option_value (char far *p, uint16_t far *v) {
  bool null = TRUE;
  if (*p++ != '=')  return FALSE;
  for (*v=0;  *p>='0' && *p<='9';  ++p)
    *v = (*v * 10) + (*p - '0'),  null = FALSE;
  return null ? FALSE : p;
}

/* parse_options */
/*   This routine will parse our line from CONFIG.SYS and extract the   */
/* driver options from it.  The routine returns TRUE if it parsed the   */
/* line successfully, and FALSE if there are any problems.  The pointer */
/* to CONFIG.SYS that DOS gives us actually points at the first char-   */
/* acter after "DEVICE=", so we have to first skip over our own file */
/* name by searching for a blank.  All the option values are stored in  */
/* global variables (e.g. DrivePort, DriveBaud, etc).          */
bool parse_options (char far *p) {
  uint16_t temp;
  while (*p!=' ' && *p!='\t' && !iseol(*p))  ++p;
  p = spanwhite(p);
  while (!iseol(*p)) {
    p = spanwhite(p);
    if (*p++ != '/')  return FALSE;
    switch (*p++) {
    case 'd':
    case 'D':
        debug = TRUE;
        cdprintf("Parsing debug as true\n");
        break;
    case 'k':
    case 'K':
        //sd_card_check = 1;
        break;
    case 'p':
    case 'P':
        if ((p=option_value(p,&temp)) == FALSE)  return FALSE;
        if ((temp < 1) || (temp > 4))
            if (debug) cdprintf("SD: Invalid partition number %d\n",temp);
        else
            partition_number = temp;
            if (debug) cdprintf("SD: partition number: %d\n", partition_number);
        break; 
    case 'b': 
    case 'B':
        if ((p=option_value(p,&temp)) == FALSE)  return FALSE;
        if ((temp < 1) || (temp > 5))
            if (debug) cdprintf("SD: Invalid port base index %x\n",temp);
        else
            portbase = temp;
            if (portbase = 1)
            {
                if (debug) cdprintf("SD: using parallel port %x\n",portbase);
            } else {
                if (debug) cdprintf("SD: using serial port %x\n",portbase);
            }
        break; 
    default:
        return FALSE;
    }
    p = spanwhite(p);
}
return TRUE;
}

static bool validate_far_ptr(void far *ptr, size_t size) {
    uint32_t linear_addr = (FP_SEG(ptr) << 4) + FP_OFF(ptr);
    return linear_addr + size <= 0x100000;  // Below 1MB
}
