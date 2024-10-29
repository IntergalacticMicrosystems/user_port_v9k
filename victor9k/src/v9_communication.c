#include <stdio.h>
#include <i86.h>
#include <dos.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>
#include <time.h>

#include "../common/protocols.h"
#include "../common/dos_device_payloads.h"
#include "../common/crc8.h"

#include "v9_communication.h"
#include "cprint.h"
#include <string.h>

extern bool debug;
static bool viaInitialized = false;

static void (__interrupt __far *originalISR)();

static volatile V9kParallelPort far *via1 = MK_FP(PHASE2_DEVICE_SEGMENT,
      VIA1_REG_OFFSET);
static volatile V9kParallelPort far *via2 = MK_FP(PHASE2_DEVICE_SEGMENT,
      VIA2_REG_OFFSET);
static volatile V9kParallelPort far *via3 = MK_FP(PHASE2_DEVICE_SEGMENT,
      VIA3_REG_OFFSET);
static volatile uint8_t far *pic = MK_FP(INTEL_DEV_SEGMENT, PIC_8259_OFFSET);

static uint8_t buffer;
static bool bufferFull = false;

void interrupt far userPortISR(void) {
   // Check if the interrupt is for your device and handle it
   if (via3->int_flag_reg > 0) {
      // Check if the interrupt is for the user port
      if (via3->int_flag_reg & CB2_INTERRUPT_MASK) {
         // Clear CB2 interrupt flag, outbound port data taken
         via3->int_flag_reg = CB2_INTERRUPT_MASK;
      } else if (via3->int_flag_reg & CA1_INTERRUPT_MASK) {
         // Inbound port Data Ready signal detected
         buffer = via3->out_in_reg_a; // Get data byte
         bufferFull = true;
         // Clear CA1 interrupt flag
         via3->int_flag_reg = CA1_INTERRUPT_MASK;
      }
      *pic = PIC_VIA_SPECIFIC_EOI; // Send End of Interrupt command to the PIC command register
      _chain_intr(originalISR);         //send the interrupt up the chain for other 6522's
   } else {
      // Call the original ISR
      _chain_intr(originalISR);
   }
}

ResponseStatus initialize_user_port(void) {
    //Via-3 is the user port or control port
    //6522 (VIA 3 CS4)
    //Memory location E8080-E808F
    cdprintf("Address of via3: %x\n", (void*)via3);
    cdprintf("via3->out_in_reg_a: %x\n", (void*)via3);

    // Save the original 6522 Interrupt Service Routine [ISR] address
    // originalISR = _dos_getvect(INTERRUPT_NUM);

    // Set my ISR
    //_dos_setvect( INTERRUPT_NUM, userPortISR );

    via3->out_in_reg_a = 0;               // out_in_reg_a is dataport, init with 0's =input bits
    cdprintf("setting data_dir_reg_a\n");
    via3->data_dir_reg_a = 0x00;          // register a is all inbound, 0000 = all bits incoming
    cdprintf("clearing out_in_reg_b\n");
    via3->out_in_reg_b = 0;               // out_in_reg_b is output, clear register
    cdprintf("setting data_dir_reg_b\n");
    via3->data_dir_reg_b = 0xFF;          // register b is all outbound, init with 1111's

    via3->int_enable_reg = VIA_CLEAR_INTERRUPTS;   //turn off all interrupts as base starting point
    via3->int_flag_reg = VIA_CLEAR_INTERRUPTS;    //clear all interrupt flags

    cdprintf("periph_ctrl_reg\n");
    via3->periph_ctrl_reg = VIA_PULSE_MODE;  // setting usage of CA/CB lines
    via3->aux_ctrl_reg = VIA_RESET_AUX_CTL;  // resets T1/T2/SR disabled, PA/PB enabled

    *pic = PIC_VIA_SPECIFIC_EOI; // Send End of Interrupt command to the PIC command register

    viaInitialized = true;

    cdprintf("Finished via_initialized\n");
    return STATUS_OK;
}

ResponseStatus sendBytes(uint8_t* data, size_t length) {
    if (viaInitialized == false) {
        cdprintf("VIA not initialized\n");
        initialize_user_port();
    }
    //cdprintf("sendBytesPB start\n");
    for (size_t i = 0; i < length; ++i) {
        cdprintf("i: %d: value: %d\n", i, data[i]);
        
        via3->out_in_reg_b = data[i]; // Send data byte
        // Wait for ACK on CB2periph_ctrl_reg
        int iteration;
        for (iteration = 0; iteration < MAX_POLLING_ITERATIONS; iteration++) {
            if (via3->int_flag_reg & CB1_INTERRUPT_MASK) {
                // Data Taken signal detected
                break;
            }
        }
        if (iteration == MAX_POLLING_ITERATIONS) {
            cdprintf("Timeout waiting for CB1 interrupt\n");
            return TIMEOUT;
        }
    }
    cdprintf("sendBytesPB end\n");
    return STATUS_OK;
}

ResponseStatus receiveBytes(uint8_t* data, size_t length) {
    if (viaInitialized == false) {
        cdprintf("VIA not initialized\n");
        initialize_user_port();
        return PORT_NOT_INITIALIZED;
   }
   cdprintf("receiveBytesPA start size: %d\n", length);
   for (size_t i = 0; i < length; ++i) {
      cdprintf("waiting for data i: %d int_flag_reg: %x\n", i, via3->int_flag_reg);
      while ((via3->int_flag_reg & CA1_INTERRUPT_MASK) == 0) {}; // Poll CA1 for Data Ready signal
      
      data[i] = via3->out_in_reg_a; // get data byte
      cdprintf("received: %d %d\n", i, data[i]);
   }
   cdprintf("receiveBytesPA end\n");
   return STATUS_OK;
}

ResponseStatus send_startup_handshake(void) {
    uint8_t handshake_count = 0;
    while (handshake_count < MAX_HANDSHAKE_ATTEMPTS) {
        handshake_count++;
        cdprintf("Handshake attempt: %d\n", handshake_count);

        if (viaInitialized == false) {
            cdprintf("VIA not initialized\n");
            ResponseStatus status = initialize_user_port();
            if (status != STATUS_OK) {
                cdprintf("Error initializing VIA\n");
                return status;
            }
        }

        via3->out_in_reg_b = STARTUP_HANDSHAKE; // Send startup handshake

        // Wait for response within timeout
        uint8_t response = 0;
        bool response_received = false;
        
        // Wait for ACK on CB2
        int iteration;
        for (iteration = 0; iteration < MAX_POLLING_ITERATIONS; iteration++) {
            if (via3->int_flag_reg & CA1_INTERRUPT_MASK) {
                response = via3->out_in_reg_a;
                    response_received = true;
                    break;
            }
            delay_us(20);
        }

        if (!response_received) {
            cdprintf("Handshake timeout, retrying\n");
            continue; // Retry handshake
        }

        if (response == HANDSHAKE_RESPONSE) {
            cdprintf("Handshake successful\n");
            return STATUS_OK;
        } else {
            cdprintf("Unexpected response %d, retrying\n", response);
            continue;
        }
    }
    cdprintf("Handshake failed after %d attempts\n", handshake_count);
    return TIMEOUT;
}

ResponseStatus send_uint16_t(uint16_t data) {
    cdprintf("send_uint16_t: %d\n", data);
    uint8_t data_array[2];
    data_array[0] = (data >> 8) & 0xFF;  //high_byte
    data_array[1] = data & 0xFF;         //low_byte;
    sendBytes( (uint8_t *) data_array, 2); 
    return STATUS_OK;
}

void sendResponseStatus(ResponseStatus status) {
    uint8_t status_value = (uint8_t)status;  // Cast enum to uint8_t
    sendBytes(&status_value, 1);               // Pass address of uint8_t
}

ResponseStatus send_command_payload(Payload *payload) {
    ResponseStatus crc_outcome;
    for (int i = 0; i < 9; i++) {
        cdprintf("sending command packet %d\n", i);
        crc_outcome = send_command_packet(payload);
        if (crc_outcome != STATUS_OK) {
            continue;
        }
        crc_outcome = send_data_packet(payload);
        if (crc_outcome != STATUS_OK) {
            continue;
        }
    }
    return crc_outcome;
}

ResponseStatus send_command_packet(Payload *payload) {
    cdprintf("sending command packet protocol: %d command: %d\n", payload->protocol, payload->command);
    sendBytes( (uint8_t *) &payload->protocol, 1);
    sendBytes( (uint8_t *) &payload->command, 1); 
    
    cdprintf("params_size: %d\n", payload->params_size);
    send_uint16_t(payload->params_size);  //send the size of the params
    cdprintf("sending params\n");
    sendBytes( payload->params, payload->params_size);  //send the actual params
    sendBytes( (uint8_t *) &payload->command_crc, 1);

    ResponseStatus crc_success = via3->out_in_reg_a;
    return crc_success;
}

ResponseStatus send_data_packet(Payload *payload) {
    cdprintf("sending data packet data_size: %d\n", payload->data_size);
    cdprintf("data_size: %d\n", payload->data_size);
    send_uint16_t(payload->data_size);  //send the size of the data
    cdprintf("sending data\n");
    sendBytes( payload->data, payload->data_size);  //send the actual data
    cdprintf("sending data_crc\n");
    sendBytes( (uint8_t *) &payload->data_crc, 1);
    ResponseStatus crc_success = via3->out_in_reg_a;
    return crc_success;
}

uint16_t receive_uint16_t() {
    uint8_t high_byte, low_byte;
    uint8_t *high_byte_ptr = (uint8_t *) &high_byte;
    uint8_t *low_byte_ptr = (uint8_t *) &low_byte;
    receiveBytes( high_byte_ptr, 1);
    receiveBytes( low_byte_ptr, 1);
    return (high_byte << 8) | low_byte;
}

ResponseStatus receive_response(Payload *response) {
    cdprintf("Receiving response\n");
    receiveBytes( (uint8_t *) &response->protocol, 1);
    receiveBytes( (uint8_t *) &response->command, 1);
    cdprintf("protocol: %d\n", response->protocol);
    response->params_size = receive_uint16_t();
    cdprintf("params_size: %d\n", response->params_size);
    receiveBytes( response->params, response->params_size);
    cdprintf("Receiving command_crc\n");
    receiveBytes( (uint8_t *) &response->command_crc, 1);
    cdprintf("command_crc: %d\n", response->command_crc);
    if (is_valid_command_crc8(response) ) {
        sendResponseStatus(STATUS_OK);
        cdprintf("command_crc valid\n");
    } else {
        sendResponseStatus(INVALID_CRC);
        cdprintf("command_crc invalid\n");
        return INVALID_CRC;
    }
    cdprintf("Receiving data size: %d\n", response->data_size);
    uint8_t expected_size = response->data_size;  // passed in the expected payload size
    response->data_size = receive_uint16_t();     // actual size of the payload
    cdprintf("data_size: %d\n", response->data_size);
    if (response->data_size > expected_size) {
        cdprintf("data_size invalid\n");
    }
    cdprintf("Receiving data\n");
    receiveBytes( (uint8_t *) response->data, response->data_size);
    cdprintf("Receiving data_crc\n");
    receiveBytes( (uint8_t *) &response->data_crc, 1);
    if (is_valid_data_crc8(response) ) {
        sendResponseStatus(STATUS_OK);
        cdprintf("data_crc valid\n");
    } else {
        sendResponseStatus(INVALID_CRC);
        cdprintf("data_crc invalid\n");
        return INVALID_CRC;
    }
    return STATUS_OK;
}
