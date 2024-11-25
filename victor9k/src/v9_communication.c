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

// static volatile V9kParallelPort far *via1 = MK_FP(PHASE2_DEVICE_SEGMENT,
//       VIA1_REG_OFFSET);
// static volatile V9kParallelPort far *via2 = MK_FP(PHASE2_DEVICE_SEGMENT,
//       VIA2_REG_OFFSET);
static volatile V9kParallelPort far *via3 = MK_FP(PHASE2_DEVICE_SEGMENT,
      VIA3_REG_OFFSET);
static volatile uint8_t far *pic = MK_FP(INTEL_DEV_SEGMENT, PIC_8259_OFFSET);

static uint8_t buffer;
static bool bufferFull = false;
static bool payloadDebug = false;

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
    if (debug) cdprintf("Address of via3: %x\n", (void*)via3);
    if (debug) cdprintf("via3->out_in_reg_a: %x\n", (void*)via3);

    // Save the original 6522 Interrupt Service Routine [ISR] address
    // originalISR = _dos_getvect(INTERRUPT_NUM);

    // Set my ISR
    //_dos_setvect( INTERRUPT_NUM, userPortISR );

    via3->out_in_reg_a = 0;               // out_in_reg_a is dataport, init with 0's =input bits
    if (debug) cdprintf("setting data_dir_reg_a\n");
    via3->data_dir_reg_a = 0x00;          // register a is all inbound, 0000 = all bits incoming
    if (debug) cdprintf("clearing out_in_reg_b\n");
    via3->out_in_reg_b = 0;               // out_in_reg_b is output, clear register
    if (debug) cdprintf("setting data_dir_reg_b\n");
    via3->data_dir_reg_b = 0xFF;          // register b is all outbound, init with 1111's

    via3->int_enable_reg = VIA_CLEAR_INTERRUPTS;   //turn off all interrupts as base starting point
    via3->int_flag_reg = VIA_CLEAR_INTERRUPTS;    //clear all interrupt flags

    if (debug) cdprintf("periph_ctrl_reg\n");
    via3->periph_ctrl_reg = VIA_PULSE_MODE;  // setting usage of CA/CB lines
    via3->aux_ctrl_reg = VIA_RESET_AUX_CTL;  // resets T1/T2/SR disabled, PA/PB enabled

    *pic = PIC_VIA_SPECIFIC_EOI; // Send End of Interrupt command to the PIC command register

    viaInitialized = true;

    if (debug) cdprintf("Finished via_initialized\n");
    return STATUS_OK;
}

//sends a byte array to the user port without waiting for DATA_TAKEN signal to
//be set before sending the next byte
ResponseStatus burstBytes(uint8_t far *data, size_t length) {
    if (viaInitialized == false) {
        if (debug) cdprintf("VIA not initialized\n");
        initialize_user_port();
    }
    if (payloadDebug) cdprintf("burstBytes start\n");
    if (payloadDebug) cdprintf("burstBytes &data: %4x:%4x\n", FP_SEG(data), FP_OFF(data));
    for (size_t i = 0; i < length; ++i) {
        via3->out_in_reg_b = data[i]; // Send data byte
    }
    if (payloadDebug) cdprintf("burstBytes end\n");
    return STATUS_OK;
}

//sends a byte array to the user port, waiting for DATA_TAKEN signal to 
//be set before sending the next byte
ResponseStatus sendBytes(uint8_t far *data, size_t length) {
    if (viaInitialized == false) {
        if (debug) cdprintf("VIA not initialized\n");
        initialize_user_port();
    }
    if (payloadDebug) cdprintf("sendBytesPB start\n");
    if (payloadDebug) cdprintf("sendBytesPB &data: %4x:%4x\n", FP_SEG(data), FP_OFF(data));
    int iteration = 0;
    for (size_t i = 0; i < length; ++i) {
        //if (debug) cdprintf("i: %d: value: %d\n", i, data[i]);
        
        via3->out_in_reg_b = data[i]; // Send data byte
        //Wait for ACK on CB2periph_ctrl_reg
        for (iteration = 0; iteration < MAX_POLLING_ITERATIONS; iteration++) {
            if (via3->int_flag_reg & CB1_INTERRUPT_MASK) {
                // Data Taken signal detected
                break;
            }
        }
        if (iteration == MAX_POLLING_ITERATIONS) {
            if (debug) cdprintf("Timeout waiting for CB1 interrupt\n");
            return TIMEOUT;
        }
    }
    //if (debug) cdprintf("sendBytesPB end\n");
    return STATUS_OK;
}

ResponseStatus receiveBytes(uint8_t far *data, size_t length) {
    if (viaInitialized == false) {
        if (debug) cdprintf("VIA not initialized\n");
        initialize_user_port();
        return PORT_NOT_INITIALIZED;
   }
   if (payloadDebug) cdprintf("receiveBytesPA start size: %d\n", length);
   if (payloadDebug) cdprintf("receiveBytesPA &data: %4x:%4x\n", FP_SEG(data), FP_OFF(data));
 
   for (size_t i = 0; i < length; ++i) {
      //if (debug) cdprintf("waiting for data i: %d int_flag_reg: %x\n", i, via3->int_flag_reg);
      while ((via3->int_flag_reg & CA1_INTERRUPT_MASK) == 0) {}; // Poll CA1 for Data Ready signal
      data[i] = via3->out_in_reg_a; // get data byte
      //if (debug) cdprintf("received: %d %d\n", i, data[i]);
   }
   if (payloadDebug) cdprintf("receiveBytesPA end\n");
   return STATUS_OK;
}

ResponseStatus send_startup_handshake(void) {
    uint8_t handshake_count = 0;
    while (handshake_count < MAX_HANDSHAKE_ATTEMPTS) {
        handshake_count++;
        if (debug) cdprintf("Handshake attempt: %d\n", handshake_count);

        if (viaInitialized == false) {
            if (debug) cdprintf("VIA not initialized\n");
            ResponseStatus status = initialize_user_port();
            if (status != STATUS_OK) {
                if (debug) cdprintf("Error initializing VIA\n");
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
            if (debug) cdprintf("Handshake timeout, retrying\n");
            continue; // Retry handshake
        }

        if (response == HANDSHAKE_RESPONSE) {
            if (payloadDebug) cdprintf("Handshake successful\n");
            return STATUS_OK;
        } else {
            if (debug) cdprintf("Unexpected response %d, retrying\n", response);
            continue;
        }
    }
    if (debug) cdprintf("Handshake failed after %d attempts\n", handshake_count);
    return TIMEOUT;
}

ResponseStatus send_uint16_t(uint16_t data) {
    if (payloadDebug) cdprintf("send_uint16_t: %d\n", data);
    uint8_t data_array[2];
    data_array[0] = (data >> 8) & 0xFF;  //high_byte
    data_array[1] = data & 0xFF;         //low_byte;
    sendBytes( (uint8_t far *) &data_array[0], 2); 
    return STATUS_OK;
}

void sendResponseStatus(ResponseStatus status) {
    uint8_t status_value = (uint8_t)status;  // Cast enum to uint8_t
    sendBytes(&status_value, 1);               // Pass address of uint8_t
}

ResponseStatus receive_response_status() {
    uint8_t status_value;
    receiveBytes( &status_value, 1);
    return (ResponseStatus)status_value;
}

ResponseStatus send_command_payload(Payload *payload) {
    ResponseStatus crc_outcome;
    for (int i = 0; i < 9; i++) {
        if (payloadDebug) cdprintf("sending command packet %d\n", i);
        crc_outcome = send_command_packet(payload);
        if (crc_outcome != STATUS_OK) {
            continue;
        }
        crc_outcome = send_data_packet(payload);
        if (crc_outcome != STATUS_OK) {
            continue;
        }
        if (payloadDebug) cdprintf("command packet sent\n");
        break;
    }
    return crc_outcome;
}

ResponseStatus send_command_packet(Payload *payload) {
    if (payloadDebug) cdprintf("sending command packet protocol: %d command: %d\n", payload->protocol, payload->command);
    sendBytes( (uint8_t far *) &payload->protocol, 1);
    sendBytes( (uint8_t far *) &payload->command, 1); 
    
    if (payloadDebug) cdprintf("params_size: %d\n", payload->params_size);
    send_uint16_t(payload->params_size);  //send the size of the params
    if (payloadDebug) cdprintf("sending params\n");
    burstBytes( payload->params, payload->params_size);  //send the actual params
    sendBytes( (uint8_t far *) &payload->command_crc, 1);

    ResponseStatus crc_success = receive_response_status();
    return crc_success;
}

ResponseStatus send_data_packet(Payload *payload) {
    if (payloadDebug) cdprintf("sending data packet data_size: %d\n", payload->data_size);
    if (payloadDebug) cdprintf("data_size: %d\n", payload->data_size);
    send_uint16_t(payload->data_size);  //send the size of the data
    if (payloadDebug) cdprintf("sending data\n");
    burstBytes( payload->data, payload->data_size);  //send the actual data
    if (payloadDebug) cdprintf("sending data_crc\n");
    sendBytes( (uint8_t far *) &payload->data_crc, 1);
    ResponseStatus crc_success = receive_response_status();
    return crc_success;
}

uint16_t receive_uint16_t() {
    uint8_t high_byte, low_byte;
    uint8_t far *high_byte_ptr = (uint8_t far *) &high_byte;
    uint8_t far *low_byte_ptr = (uint8_t far *) &low_byte;
    receiveBytes( high_byte_ptr, 1);
    receiveBytes( low_byte_ptr, 1);
    return (high_byte << 8) | low_byte;
}

ResponseStatus receive_response(Payload *response) {
    if (payloadDebug) cdprintf("Receiving response\n");
    receiveBytes( (uint8_t *) &response->protocol, 1);
    receiveBytes( (uint8_t *) &response->command, 1);
    if (payloadDebug) cdprintf("protocol: %d\n", response->protocol);
    response->params_size = receive_uint16_t();
    if (payloadDebug) cdprintf("params_size: %d\n", response->params_size);
    receiveBytes( response->params, response->params_size);
    if (payloadDebug) cdprintf("Receiving command_crc\n");
    receiveBytes( (uint8_t *) &response->command_crc, 1);
    if (payloadDebug) cdprintf("command_crc: %d\n", response->command_crc);
    if (is_valid_command_crc8(response) ) {
        sendResponseStatus(STATUS_OK);
        if (payloadDebug) cdprintf("command_crc valid\n");
    } else {
        sendResponseStatus(INVALID_CRC);
        if (debug) cdprintf("command_crc invalid\n");
        return INVALID_CRC;
    }
    if (payloadDebug) cdprintf("Receiving data size: %d\n", response->data_size);
    response->data_size = receive_uint16_t();    
    if (payloadDebug) cdprintf("data_size: %d\n", response->data_size);
    if (payloadDebug) cdprintf("Receiving data\n");
    receiveBytes( response->data, response->data_size);
    if (payloadDebug) cdprintf("Receiving data_crc\n");
    receiveBytes( (uint8_t far *) &response->data_crc, 1);
    if (is_valid_data_crc8(response) ) {
        sendResponseStatus(STATUS_OK);
        if (payloadDebug) cdprintf("data_crc valid\n");
    } else {
        sendResponseStatus(INVALID_CRC);
        if (debug) cdprintf("data_crc invalid\n");
        return INVALID_CRC;
    }
    return STATUS_OK;
}
