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

#include <stdint.h>
#include <stddef.h>
#include <dos.h>
#include <string.h>

#include "device.h"
#include "diskio.h"
#include "devinit.h"
#include "template.h"
#include "cprint.h"     /* Console printing direct to hardware */
#include "v9_communication.h"  /* Victor 9000 user port to raspberry pico communication protocol */
#include "../../common/protocols.h"
#include "../../common/dos_device_payloads.h"
#include "../../common/crc8.h"

#ifdef USE_INTERNAL_STACK

static uint8_t our_stack[STACK_SIZE];
uint8_t *stack_bottom = our_stack + STACK_SIZE;
uint32_t dos_stack;

#endif // USE_INTERNAL_STACK

static bool validate_far_ptr(void far *ptr, size_t size);

bool initNeeded = TRUE;
int8_t num_drives = -1;

//BPB table is an array of near pointers to an array BPB structures
//DOS passes around a far pointer to the table
bpb my_bpbs[MAX_IMG_FILES] = {0};  // Array of BPB instances
bpb near *my_bpb_tbl[MAX_IMG_FILES] = {NULL};     // BPB Table = array of near pointers to BPB structures
bpb * __far *my_bpb_tbl_far_ptr = (bpb * __far *)my_bpb_tbl;   // Far pointer to the BPB table

#ifdef RAMDRIVE
MiniDrive myDrive = {0};
#endif

extern bool debug;



request __far *fpRequest = (request __far *)0;

static uint16_t open( void )
{
    if (debug) cdprintf("SD: -----------------------------------------------\n");
    if (debug) cdprintf("SD: open()\n");
    return S_DONE;
}

static uint16_t close( void )
{ 
    if (debug) cdprintf("SD: -----------------------------------------------\n");
    if (debug) cdprintf("SD: close()\n");
    return S_DONE;
} 

/* mediaCheck */
/*    DOS calls this function to determine if the tape in the drive has   */
/* been changed.  The SD hardware can't determine this (like many      */
/* older 360K floppy drives), and so we always return the "Don't Know" */
/* response.  This works well enough...                                */
static uint16_t mediaCheck (void)
{
  struct ALL_REGS registers;
  get_all_registers(&registers);
  
  // mediaCheck_data far *media_ptr;
  uint8_t drive_num = fpRequest->r_unit;
  if (debug) cdprintf("SD: -----------------------------------------------\n");
  if (debug) cdprintf("SD: mediaCheck()\n");
  if (debug) cdprintf("about to parse mediaCheck, ES: %x BX: %x\n", registers.es, registers.bx);
  if (debug) cdprintf("SD: mediaCheck: unit=%x\n", fpRequest->r_unit);
  if (debug) cdprintf("SD: mediaCheck: fpRequest=%x:%x\n", FP_SEG(fpRequest), FP_OFF(fpRequest));
  if (debug) cdprintf("SD: mediaCheck: &my_bpb_tbl[0]=%x:%x\n", FP_SEG(&my_bpb_tbl[drive_num]), FP_OFF(&my_bpb_tbl[drive_num]));
  
  
  if (debug) cdprintf("SD: mediaCheck(): r_unit 0x%2xh media_descriptor = 0x%2xh r_mc_red_code: %d fpRequest: %x:%x\n", 
    fpRequest->r_unit, fpRequest->r_mc_media_desc, M_NOT_CHANGED,
    FP_SEG(fpRequest), FP_OFF(fpRequest));
 
  fpRequest->r_mc_ret_code = M_NOT_CHANGED;
  //fpRequest->r_mc_ret_code = sd_mediaCheck(*fpRequest->r_mc_vol_id) ? M_CHANGED : M_NOT_CHANGED;
  return S_DONE;
}

/* buildBpb */
/*   DOS uses this function to build the BIOS parameter block for the   */
/* specified drive.  For diskettes, which support different densities   */
/* and formats, the driver actually has to read the BPB from the boot   */
/* sector on the disk.  */
static uint16_t buildBpb (void)
{
  uint8_t drive_num = fpRequest->r_unit;
  uint8_t media_descriptor = fpRequest->r_bpmdesc;
  if (debug) cdprintf("SD: -----------------------------------------------\n");
  if (debug) cdprintf("SD: buildBpb() unit=%x\n", drive_num);
  if (debug) cdprintf("SD: buildBpb: fpRequest=%x:%x\n", FP_SEG(fpRequest), FP_OFF(fpRequest));
  if (debug) cdprintf("SD: buildBpb: my_bpb_tbl[%u]=%x:%x\n", (uint16_t) drive_num,
     FP_SEG(my_bpb_tbl[drive_num]), FP_OFF(my_bpb_tbl[drive_num]));
  
  if (debug) cdprintf("SD: buildBpb(): media_descriptor=0x%2xh r_bpfat: %x:%x r_bpb_ptr: %x:%x my_bpb: %x:%x\n", 
      (uint16_t) media_descriptor, FP_SEG(fpRequest->r_bpfat), FP_OFF(fpRequest->r_bpfat),
      FP_SEG(fpRequest->r_bpb_ptr), FP_OFF(fpRequest->r_bpb_ptr),
      FP_SEG(&my_bpb_tbl[drive_num]), FP_OFF(&my_bpb_tbl[drive_num]));
  //we build the BPB during the deviceInit() method. just return pointer to built table
  bpb far *bpb_cast_ptr = MK_FP(FP_SEG(my_bpb_tbl[drive_num]), FP_OFF(my_bpb_tbl[drive_num]));
  fpRequest->r_bpb_ptr = bpb_cast_ptr;

  return S_DONE;
}

static uint16_t IOCTLInput(void)
{
    struct ALL_REGS regs;
    
    get_all_registers(&regs);

    //for the Victor disk IOCTL the datastructure is passed on thd DS:DX registers
    V9kDiskInfo far *v9k_disk_info_ptr = MK_FP(regs.ds, regs.dx);
    if (debug) cdprintf("SD: -----------------------------------------------\n");
    if (debug) cdprintf("SD: IOCTLInput()\n");
    cdprintf("SD: IOCTLInput()");
    cdprintf("SD: IOCTLInput(): di_ioctl_type = 0x%xh\n", v9k_disk_info_ptr->di_ioctl_type);
    {
        switch (v9k_disk_info_ptr->di_ioctl_type)
        {
        case GET_DISK_DRIVE_PHYSICAL_INFO:
            if (debug) cdprintf("SD: IOCTLInput() - GET_DISK_DRIVE_PHYSICAL_INFO\n");
            // cdprintf("SD: IOCTLInput() -AH — IOCTL function number (44h) %x", registers.ax);
            // cdprintf("SD: IOCTLInput() -AL — IOCTL device driver read request value (4) %x", registers.ax);
            // cdprintf("SD: IOCTLInput() -BL — drive (0 = A, 1 = B, etc.) %x", registers.bx);
            // cdprintf("SD: IOCTLInput() -CX — length in bytes of this request structure (6) %x", registers.cx);
            // cdprintf("SD: IOCTLInput() -DS:DX — pointer to data structure %x:%x", registers.ds, registers.dx);

            v9k_disk_info_ptr->di_ioctl_type = GET_DISK_DRIVE_PHYSICAL_INFO;
            bool failed, hard_disk, left;
            failed = false;         /* 0 = success, 1 = failure, so when failed=false that means success=true */
            hard_disk = true;
            left = false;
            v9k_disk_info_ptr->di_ioctl_status = failed;
            v9k_disk_info_ptr->di_disk_type = hard_disk;
            v9k_disk_info_ptr->di_disk_location = left;

            if (debug) cdprintf("SD: IOCTLInput() - GET_DISK_DRIVE_PHYSICAL_INFO - failed: %d\n", (int16_t) failed);

            return S_DONE;
            break;

        default:
            failed = true;
            v9k_disk_info_ptr->di_ioctl_status = failed;
            return (S_DONE | S_ERROR | E_UNKNOWN_COMMAND);
            break;
        }
    }

    return (S_DONE | S_ERROR | E_HEADER_LENGTH);
}

/* dosError */
/*   This routine will translate a SD error code into an appropriate  */
/* DOS error code.  This driver never retries on any error condition.   */
/* For actual tape read/write errors it's pointless because the drive   */
/* will have already tried several times before reporting the failure.  */
/* All the other errors (e.g. write lock, communications failures, etc) */
/* are not likely to succeed without user intervention, so we go thru   */
/* the usual DOS "Abort, Retry or Ignore" dialog. Communications errors */
/* are a special situation.  In these cases we also set global flag to  */
/* force a controller initialization before the next operation.      */
int dosError (int status)
{
  switch (status) {
    case RES_OK:     return 0;
    case RES_WRPRT:  return E_WRITE_PROTECT;
    case RES_NOTRDY: initNeeded = false; return E_NOT_READY;               
    case RES_ERROR:  initNeeded = false; return E_SECTOR_NOT_FND;
    case RES_PARERR: return E_CRC_ERROR;

    default:
    if (debug) cdprintf("SD: unknown drive error - status = 0x%2x\n", (uint16_t) status);
        return E_GENERAL_FAILURE;
  }
}

unsigned get_stackpointer();
#pragma aux get_stackpointer = \
    "mov ax, sp" \
    value [ax];

/* Read Data */

static uint16_t readBlock (void)
{
    if (debug) cdprintf("SD: -----------------------------------------------\n");
    if (debug) cdprintf("SD: readBlock()\n");
    
    uint16_t sector_count = fpRequest->r_count;
    uint16_t start_sector = fpRequest->r_start;
    uint8_t media_descriptor = fpRequest->r_meddesc;
    uint8_t far *transfer_area = (uint8_t far *)fpRequest->r_trans;

    // Validate the transfer buffer
    if (!validate_far_ptr(transfer_area, sector_count * SECTOR_SIZE)) {
        if (debug) cdprintf("SD: Invalid transfer buffer address\n");
        return (S_DONE | S_ERROR | E_GENERAL_FAILURE);
    }
    if (debug) {
      if (debug) cdprintf("SD: read block: media_descriptor=0x%2xh, start=%u, count=%u, r_trans=%x:%x\n",
       (uint16_t) media_descriptor, start_sector, sector_count, 
                FP_SEG(fpRequest->r_trans), FP_OFF(fpRequest->r_trans));
      if (debug) cdprintf("SD: read block: media_descriptor=0x%2xh, start=%u, count=%u, r_trans=%x:%x\n",
        (uint16_t) media_descriptor, start_sector, sector_count, 
                FP_SEG(fpRequest->r_trans), FP_OFF(fpRequest->r_trans));
    }

    //Prepare satic payload Params
    Payload readPayload = {0};
    readPayload.protocol = SD_BLOCK_DEVICE;
    readPayload.command = READ_BLOCK;

    // Prepare static read parameters
    ReadParams readParams = {0};
    readParams.drive_number = fpRequest->r_unit;
    readParams.media_descriptor = media_descriptor;

    readPayload.params_size = sizeof(readParams);
    readPayload.params = (uint8_t *)(&readParams);
    uint8_t data[1] = {0};
    uint8_t far *data_ptr = &data[0];
    readPayload.data = data_ptr;
    readPayload.data_size = sizeof(data);
    if (debug) cdprintf("sending data_size: %u\n", readPayload.data_size);

      // Prepare dynamic read parameters
      readParams.sector_count = sector_count;
      readParams.start_sector = start_sector;
      create_payload_crc8(&readPayload);
      
      ResponseStatus outcome = send_command_payload(&readPayload);
      if (outcome != STATUS_OK) {
          cdprintf("Error: Failed to send READ_BLOCK command to SD Block Device. Outcome: %d\n",(uint16_t) outcome);
          return (S_DONE | S_ERROR | E_UNKNOWN_MEDIA );
      } 
  
      //getting the response from the pico
      if (debug) cdprintf("command sent success, starting receive response\n");

      Payload responsePayload = {0};
      uint8_t response_params[3] = {0};
      responsePayload.params = &response_params[0];
      responsePayload.data = transfer_area;
      responsePayload.data_size = sector_count * SECTOR_SIZE;
      outcome = receive_response(&responsePayload);
      if (outcome != STATUS_OK) {
          cdprintf("SD Error: Failed to receive response from SD Block Device %d\n", (uint16_t) outcome);
          return (S_DONE | S_ERROR | E_UNKNOWN_MEDIA );
      }

    #ifdef RAMDRIVE
    //return data from RAM drive
    Sector *startSector = &myDrive.sectors[start_sector]; 

    // Calculate the number of bytes to copy
    unsigned int numBytes = SECTOR_SIZE * sector_count;

    // Perform the copy
    _fmemcpy(fpRequest->r_trans, startSector, numBytes);
    #endif
    
    return (S_DONE);
}


/* Write Data */
/* Write Data with Verification */
static uint16_t write_block (bool verify)
{
  if (debug) cdprintf("SD: -----------------------------------------------\n");
  if (debug) cdprintf("SD: write block: verify=%u\n", (uint16_t) verify);
  uint16_t sector_count = fpRequest->r_count;
  uint16_t start_sector = fpRequest->r_start;
  uint8_t media_descriptor = fpRequest->r_meddesc;
  uint8_t far *transfer_area = (uint8_t far *)fpRequest->r_trans;
  if (debug) {
    if (debug) cdprintf("SD: write block: media_descriptor=0x%2xh, start=%u, count=%u, r_trans=%x:%x\n",
      (uint16_t) media_descriptor, start_sector, sector_count, 
              FP_SEG(transfer_area), FP_OFF(transfer_area));
    if (debug) cdprintf("SD: write block: media_descriptor=0x%2xh, start=%u, count=%u, r_trans=%x:%x\n",
      (uint16_t) media_descriptor, start_sector, sector_count, 
              FP_SEG(transfer_area), FP_OFF(transfer_area));
  }

  if (initNeeded)  return (S_DONE | S_ERROR | E_NOT_READY); //not initialized yet
 
  //Prepare satic payload Params
    Payload writePayload = {0};
    writePayload.protocol = SD_BLOCK_DEVICE;

    if (verify) {
      writePayload.command = WRITE_VERIFY;
    } else {
      writePayload.command = WRITE_NO_VERIFY;
    }

    // Prepare static read parameters
    WriteParams writeParams = {0};
    writeParams.drive_number = fpRequest->r_unit;
    writeParams.media_descriptor = media_descriptor;
    writeParams.sector_count = sector_count;
    writeParams.start_sector = start_sector;

    writePayload.params_size = sizeof(writeParams);
    writePayload.params = (uint8_t *)(&writeParams);
    writePayload.data_size = sector_count * SECTOR_SIZE;
    writePayload.data = transfer_area;
    
    if (debug) cdprintf("sending data_size: %u\n", (uint16_t) writePayload.data_size);
    create_payload_crc8(&writePayload);

    ResponseStatus outcome = send_command_payload(&writePayload);
    if (outcome != STATUS_OK) {
        cdprintf("Error: Failed to send READ_BLOCK command to SD Block Device. Outcome: %u\n", (uint16_t) outcome);
        return (S_DONE | S_ERROR | E_UNKNOWN_MEDIA );
    } 
  
    //getting the response from the pico
    cdprintf("command sent success, starting receive response\n");

    Payload responsePayload = {0};
    uint8_t response_params[3] = {0};
    responsePayload.params = &response_params[0];
    responsePayload.data = transfer_area;
    responsePayload.data_size = sector_count * SECTOR_SIZE;
    outcome = receive_response(&responsePayload);
    if (outcome != STATUS_OK) {
        cdprintf("SD Error: Failed to receive response from SD Block Device %u\n", (uint16_t) outcome);
        return (S_DONE | S_ERROR | E_UNKNOWN_MEDIA );
    }

    #ifdef RAMDRIVE
    unsigned int numBytes = SECTOR_SIZE * sector_count;
    Sector *startSector = &myDrive.sectors[start_sector];

    // Perform the copy from the DOS buffer to the MiniDrive
    _fmemcpy(startSector->data, fpRequest->r_trans, numBytes);
    #endif

  return (S_DONE);
}

static uint16_t writeNoVerify () {
    if (debug) cdprintf("SD: -----------------------------------------------\n");
    if (debug) cdprintf("SD: writeNoVerify()\n");
    return write_block(FALSE);
}

static uint16_t writeVerify () {
    if (debug) cdprintf("SD: -----------------------------------------------\n");
    if (debug) cdprintf("SD: writeVerify()\n");
    return write_block(TRUE);
}

static bool isMyUnit(int8_t unitCode) {
  if (debug) cdprintf("SD: -----------------------------------------------\n");
  if (debug) cdprintf("SD: isMyUnit()\n");
  if (debug) cdprintf("SD: isMyUnit(): unitCode: %u num_drives: %u\n", (uint16_t) unitCode, (uint16_t) num_drives);
  if (unitCode <= num_drives) {
    return true;
  } else {
    return false;
  }
}

static driverFunction_t dispatchTable[] =
{
    deviceInit,          // 0x00 Initialize
    mediaCheck,          // 0x01 MEDIA Check
    buildBpb,            // 0x02 Build BPB
    IOCTLInput,          // 0x03 Ioctl In
    readBlock,           // 0x04 Input (Read)
    NULL,                // 0x05 Non-destructive Read
    NULL,                // 0x06 Input Status
    NULL,                // 0x07 Input Flush
    writeNoVerify,       // 0x08 Output (Write)
    writeVerify,         // 0x09 Output with verify
    NULL,                // 0x0A Output Status
    NULL,                // 0x0B Output Flush
    NULL,                // 0x0C Ioctl Out
    open,                // 0x0D Device Open
    close,               // 0x0E Device Close
    NULL,                // 0x0F Removable MEDIA
    NULL,                // 0x10 Output till busy
    NULL,                // 0x11 Unused
    NULL,                // 0x12 Unused
    NULL,                // 0x13 Generic Ioctl
    NULL,                // 0x14 Unused
    NULL,                // 0x15 Unused
    NULL,                // 0x16 Unused
    NULL,                // 0x17 Get Logical Device
    NULL,                // 0x18 Set Logical Device
    NULL                 // 0x19 Ioctl Query
};

static driverFunction_t currentFunction;

void __far DeviceInterrupt( void )
#pragma aux DeviceInterrupt __parm []
{
#ifdef USE_INTERNAL_STACK
    switch_stack();
#endif

    push_regs();

    if ( fpRequest->r_command > C_MAXCMD || NULL == (currentFunction = dispatchTable[fpRequest->r_command]) )
    {
        fpRequest->r_status = S_DONE | S_ERROR | E_UNKNOWN_COMMAND;
    }
    else
    {
        // writeToDriveLog("SD: DeviceInterrupt command: %d r_unit: 0x%2xh isMyUnit(): %d r_status: %d r_length: %d initNeeded: %d\n",
        //     fpRequest->r_command, fpRequest->r_unit, isMyUnit(fpRequest->r_unit), fpRequest->r_status, fpRequest->r_length, initNeeded);   
        if ((initNeeded && fpRequest->r_command == C_INIT) || isMyUnit(fpRequest->r_unit)) {
            fpRequest->r_status = currentFunction();
        } else {
            // This is  not for me to handle
            struct device_header __far *deviceHeader = MK_FP(getCS(), 0);
            struct device_header __far *nextDeviceHeader = deviceHeader->dh_next;
            nextDeviceHeader->dh_interrupt();
        }
    }

    pop_regs();

#ifdef USE_INTERNAL_STACK
    restore_stack();
#endif
}

void __far DeviceStrategy( request __far *req )
#pragma aux DeviceStrategy __parm [__es __bx]
{
    fpRequest = req;
     // writeToDriveLog("SD: DeviceStrategy command: %d r_unit: %d r_status: %d r_length: %x fpRequest: %x:%x\n",
     //       req->r_command, req->r_unit, req->r_status, req->r_length, FP_SEG(fpRequest), FP_OFF(fpRequest));

}

static bool validate_far_ptr(void far *ptr, size_t size) {
    uint32_t linear_addr = (FP_SEG(ptr) << 4) + FP_OFF(ptr);
    return linear_addr + size <= 0x100000;  // Below 1MB
}
