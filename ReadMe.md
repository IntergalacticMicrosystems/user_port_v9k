
# Victor 9000 User Port SD Card Interface

## Overview

This project enables the Victor 9000 to interface with an SD card via the User Port using a Raspberry Pi Pico. The Pico acts as a bridge between the Victor 9000 and the SD card, allowing MS-DOS 3.1 to read and write files as if it were a standard storage device.

The project consists of:
- 	A custom PCB that connects a Raspberry Pi Pico to the Victor 9000 User Port.
- 	A DOS device driver for MS-DOS 3.1 on the Victor 9000.
- 	Firmware for the Raspberry Pi Pico to handle SD card operations.

⸻

Features
- 	Read and write FAT16/FAT32 formatted SD cards.
- 	Accessible as a drive letter under MS-DOS 3.1.
- 	Uses the Victor 9000 User Port for communication.
- 	Low power consumption – powered directly from the user port.

⸻

Hardware Requirements

Required Components
- 	Victor 9000 running MS-DOS 3.1.
- 	Custom PCB
- 	Raspberry Pi Pico.
- 	SD card slot.
- 	SD card formatted as FAT16 or FAT32.

See the schamtic for Victor 9000 User Port to Raspberry Pi Pico Pin Mapping


⸻

Software Installation

1. Flash the Raspberry Pi Pico Firmware
    1. Connect your Pico to a PC via USB.
    1. Hold the BOOTSEL button and power on the Pico.
    1. Drag-and-drop the compiled firmware (user_port_pico.uf2) onto the RPI-RP2 drive.

1. Install the MS-DOS Device Driver
    1.	Copy the Victor 9000 DOS driver (SDDRV.SYS) to the system drive.
    1.	Edit CONFIG.SYS to include: `DEVICE=userport.sys`

1.	Reboot the Victor 9000.

⸻

Usage

Accessing the SD Card

Once the driver is loaded, the SD card will be assigned a drive letter (e.g., E:). You can then use standard DOS commands:

DIR E:\
COPY C:\FILE.TXT E:\
DEL E:\OLD.TXT

Formatting an SD Card

The SD card must be formatted with FAT16 or FAT32. Use a modern PC to format the card before inserting it into the Victor 9000. You'll also need a Victor formatted disk image loaded on the card.

⸻

Troubleshooting

Driver Not Loading
 - Ensure userport.sys is in the root of your boot disk
 - Check CONFIG.SYS syntax.
 - Verify the Victor 9000 User Port connection.


SD Card Not Detected
 - Ensure the SD card is FAT16/FAT32 formatted.
 - Check the Pico firmware is flashed correctly.
 - Verify the Victor 9000 User Port wiring.
## Credits
 - Hardware & Software Development: Paul Devine
- Many thanks to profdc9 at the VCFED forums who provided the code that got me started you can find it here:
[ https://forum.vcfed.org/index.php?threads/ms-dos-driver-for-an-sd-card-connected-via-a-parallel-port.41516/](url)
- Many thanks to Educardo Casino for his Openwatcom DOS driver [https://github.com/eduardocasino/dos-device-driver](url) which I incorported.
- Victor 9000 Background info and help: Many thanks to Bitsavers, Disk Blitz and the folks at the AppleSauce discord for patiently explaining to me lots of background; FozzTexx for his help in understanding some of the Victor 9000 hardware; FreddyV, Polpo, and the folks at the Retro Pico discord for letting me lurk over their shoulders; and Intergalactic for finding the hard disk struct documentation!

For questions or contributions, feel free to open an issue or submit a pull request.

I'm releasing this under the GPL-v2 as the source code I've incorporated utilized that licensing scheme.