#include "../../common/protocols.h"

#ifndef _USER_PORT_H
#define _USER_PORT_H

#pragma pack(push, 1)
// Serial port control registers
#define INTEL_DEV_SEGMENT 0xE000      // Base address segment for UART control, 7201 serial controller
#define NEC7201_REG_OFFSET 0x0040     // Base address offset for UART control, 7201 serial controller
#define NEC7201_RESET_CMD 0x18        // Channel reset command for the 7201 serial controller in hex
#define NEC7201_CHAR_AVAILABLE 0x01   // Bit mask for character available in 7201 status register
#define NEC7201_TX_BUFFER_EMPTY 0x04  // Bit mask for TX buffer empty in 7201 status register

// Timer control registers, used to control baud rate
#define TIMER_8253_OFFSET 0x0020 // Timer base address offset for baud rate setup
#define CLOCK_8253_FREQUENCY 1250000 // 1.25MHz clock frequency

// Segment for 8259 programable interrupt controller [PIC] used for interrupt handling
#define PIC_8259_OFFSET 0x0000
#define PIC_NON_SPECIFIC_EOI  0x20   //Mask for End of Interrupt
#define PIC_VIA_SPECIFIC_EOI 0x62   //Mask for End of Interrupt

// Constants for setting up the interrupt service routine
#define INTERRUPT_NUM 0x43
#define IVT_ENTRY (INTERRUPT_NUM * 4) // Each Interrupt Vector Table [IVT] entry is 4 bytes


// The keyboard VIA, resgister 1, is located at E804:0001. The appropriate bits are:
//   Bit 0 (PA0) for port A
//   Bit 1 (PAl) for port B
// Thus,bysettingPA0 to zero,the internal clock is enabled for port A;
// setting PA1 to zero will enable the internal clock for port B.
// Setting PA0 or PAl to one will enable the external disabling the internal clock.
#define PHASE2_DEVICE_SEGMENT 0xE800
#define VIA1_REG_OFFSET   0x0020    // via1 for centronics parallel port   
#define VIA2_REG_OFFSET   0x0040    // via2 keyboard, serial params, parallel enable
#define VIA3_REG_OFFSET   0x0080    // via3 user port, GPIO pins on motherboard
#define TALK_ENABLE_H     0x01      // patallel talk-enable line line VIA2 PB0
#define INTERNAL_EXT_A    0x01      // PAO INT/EXTA Serial A internal / external clock select (LOW=INT)
#define INTERNAL_EXT_B    0x02      // PA1 INT/EXTB Serial B internal / external clock select (LOW=INT)
#define SERIAL_A_RING_IND 0x03      // PA2 RIA Serial A ring indicator (J8-22)
#define SERIAL_A_DSR      0x04      // PA3 DSRA Serial A data set ready (J8-6)
#define SERIAL_B_RING_IND 0x05      // PA4 RIB Serial B ring indicator (J9-22)
#define SERIAL_B_DSR      0x06      // PA5 DSRB Serial B data set ready (J9-6)

// Constants for Handshake configuration
#define VIA_PULSE_MODE         0xAA   // Setup CA / CB as pulse lines for read/write
#define VIA_HANDSHAKE_MODE     0x88   // Setup CA / CB as handshake lines for read/write

// Constants for setting up the Shift Register [SR] and interrupts
#define SR_RESET_MODE          0xE3  // Bit mask to clear the SR mode bits
#define SHIFT_OUT_MODE_110     0x18  // Shift out with mode 110
#define SR_INTERRUPT_MASK      0x04  // Bit to enable SR interrupts in int_enable_reg

//Constants for overall 6522 interactions
#define CA1_INTERRUPT_MASK     0x02  // Bit to enable CA1 interrupts in int_enable_reg
#define CA2_INTERRUPT_MASK     0x01  // Bit to enable CA2 interrupts in int_enable_reg
#define CB1_INTERRUPT_MASK     0x10  // Bit to enable CB1 interrupts in int_enable_reg
#define CB2_INTERRUPT_MASK     0x04  // Bit to enable CB2 interrupts in int_enable_reg
#define CA1_CB1_BOTH_INTERRUPT 0x12  // Bit pattern to enable both CA1 and CB1 interrupts for handshake
#define INTERRUPT_ENABLE       0x80  // Bit in the int_enable_reg to turn on interrupts 
#define VIA_CLEAR_INTERRUPTS   0x7F  // Bit pattern to clear entire interrupt register
#define VIA_RESET_AUX_CTL      0x03  // Resets T1/T2/SR disabled, PA/PB latching enabled
#define MAX_POLLING_ITERATIONS 5000  // Maximum number of iterations to poll for interrupt
#define MAX_HANDSHAKE_ATTEMPTS 100   // Maximum number of handshake attempts before timeout

enum ports {PARALLEL, SERIAL_A, SERIAL_B, USER_PORT};


typedef struct {
    uint8_t out_in_reg_b;
    uint8_t out_in_reg_a;
    uint8_t data_dir_reg_b;         // data-direction, reg 'b'
    uint8_t data_dir_reg_a;
    uint8_t timer1_ctr_lo;
    uint8_t timer1_ctr_hi;
    uint8_t timer1_latch_lo;
    uint8_t timer1_latch_hi;
    uint8_t timer2_ctr_lo;
    uint8_t timer2_ctr_hi;
    uint8_t shift_reg;
    uint8_t aux_ctrl_reg;           // auxiliary Ctrl reg
    uint8_t periph_ctrl_reg;
    uint8_t int_flag_reg;           // interrupt flag register
    uint8_t int_enable_reg;         // interrupt enable register
    uint8_t out_in_reg_a_no_hs;     // out-in reg 'a' NO HANDSHAKE
} V9kParallelPort;

/* something about our makefile isn't letting these be linked, defining here */
extern void Enable( void );
#pragma aux Enable = \
    "sti" \
    parm [] \
    modify [];
extern void Disable( void );
#pragma aux Disable = \
    "cli" \
    parm [] \
    modify [];

static void delay_us(unsigned int n);
#pragma aux delay_us = \
    "mov cx, ax", \
    "loopit:", \
    "loop loopit", \
    parm [ax] \
    modify [cx];

#pragma pack( pop )

ResponseStatus initialize_user_port(void);
void interrupt far userPortISR(void);
ResponseStatus send_startup_handshake(void);
ResponseStatus send_uint16_t(uint16_t data);
ResponseStatus sendBytes(uint8_t* data, size_t length);
ResponseStatus receiveBytes(uint8_t far *data, size_t length);
ResponseStatus send_command_payload(Payload *payload);
ResponseStatus send_command(Payload *command);
ResponseStatus receive_response(Payload *response);
ResponseStatus send_command_packet(Payload *payload);
ResponseStatus send_data_packet(Payload *payload);

#endif
