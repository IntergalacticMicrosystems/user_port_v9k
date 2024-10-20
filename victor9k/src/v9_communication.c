#include <stdio.h>
#include <string.h>
#include <i86.h>
#include <dos.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>
#include <time.h>

#include "../common/protocols.h"
#include "../common/dos_device_payloads.h"

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

ResponseStatus initialize_user_port(void) {
    //Via-3 is the user port or control port
    //6522 (VIA 3 CS4)
    //Memory location E8080-E808F
    printf("Address of via3: %x\n", (void*)via3);
    printf("via3->out_in_reg_a: %x\n", (void*)via3);

    // Save the original 6522 Interrupt Service Routine [ISR] address
    originalISR = _dos_getvect(INTERRUPT_NUM);

    // Set my ISR
   _dos_setvect( INTERRUPT_NUM, userPortISR );

    via3->out_in_reg_a = 0;               // out_in_reg_a is dataport, init with 0's =input bits
    printf("data_dir_reg_a\n");
    via3->data_dir_reg_a = 0x00;          // register a is all inbound, 0000 = all bits incoming
    printf("out_in_reg_b\n");
    via3->out_in_reg_b = 0;               // out_in_reg_b is output, clear register
    printf("data_dir_reg_b\n");
    via3->data_dir_reg_b = 0xFF;          // register b is all outbound, init with 1111's

    via3->int_enable_reg = VIA_CLEAR_INTERRUPTS;   //turn off all interrupts as base starting point
    via3->int_flag_reg = VIA_CLEAR_INTERRUPTS;    //clear all interrupt flags

    printf("periph_ctrl_reg\n");
    via3->periph_ctrl_reg = VIA_PULSE_MODE;  // setting usage of CA/CB lines
    via3->aux_ctrl_reg = VIA_RESET_AUX_CTL;  // resets T1/T2/SR disabled, PA/PB enabled

    *pic = PIC_VIA_SPECIFIC_EOI; // Send End of Interrupt command to the PIC command register

    viaInitialized = true;

    printf("Finished via_initialized\n");
}

void sendBytes(uint8_t* data, size_t length) {
    if (viaInitialized == false) {
        printf("VIA not initialized\n");
        initialize_user_port();
        return;
    }
    //printf("sendBytesPB start\n");
    for (size_t i = 0; i < length; ++i) {
        //printf("%d: %d\n", i, data[i]);
        sendAcknowledged = false;
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
            bool recovered = false;
            // Timeout reached without detecting Data Taken signal, reset CB2
            printf("data timeout while sending: %d reseting CB2\n", i);
            via3->out_in_reg_b = data[i]; // Send data byte again
            for (iteration = 0; iteration < (MAX_POLLING_ITERATIONS / 10); iteration++) {
                if (via3->int_flag_reg & CB1_INTERRUPT_MASK) {
                    // Data Taken signal detected
                    recovered = true;
                    break;
                }
            }
            if (recovered == false) {
                printf("data timeout while sending: %d\n", i);
                protocolTimeout = true;
                return;
            }

        }
    }
    printf("sendBytesPB end\n");
}

void receiveBytes(uint8_t* data, size_t length) {
    if (viaInitialized == false) {
        printf("VIA not initialized\n");
        initialize_user_port();
        return;
   }
   printf("receiveBytesPA start\n");
   for (size_t i = 0; i < length; ++i) {
      while ((via3->int_flag_reg & CA1_INTERRUPT_MASK) == 0) {}; // Poll CA1 for Data Ready
      //while (!dataAvailable) { }
      
      data[i] = via3->out_in_reg_a; // get data byte
      //printf("received: %d %d\n", i, data[i]);
      dataAvailable = false;
   }
   printf("receiveBytesPA end\n");
}

ResponseStatus send_command_payload(Payload *payload) {
    ResponseStatus crc_outcome = send_command_packet(payload);
    if (crc_outcome != STATUS_OK) {
        return crc_outcome;
    }
    crc_outcome = send_data_packet(payload);
    return crc_outcome;
}

ResponseStatus send_command_packet(Payload *payload) {
    sendBytes( (uint8_t *) &payload->protocol, 1);
    sendBytes( (uint8_t *) &payload->command, 1); 
    sendBytes( (uint8_t *) &(payload->params_size), 2); 
    sendBytes( payload->params, payload->params_size);
    sendBytes( (uint8_t *) &payload->command_crc, 1);
    ResponseStatus crc_success = via3->out_in_reg_a;
    return crc_success;
}

ResponseStatus send_data_packet(Payload *payload) {
    sendBytes( (uint8_t *) &payload->data_size, 2);
    sendBytes( payload->data, payload->data_size); 
    sendBytes( (uint8_t *) &payload->data_crc, 1);
    ResponseStatus crc_success = via3->out_in_reg_a;
    return crc_success;
}

ResponseStatus receive_response(Payload *response) {
    receiveBytes( (uint8_t *) &response->protocol, 1);
    receiveBytes( (uint8_t *) &response->command, 1);

    uint8_t high_byte, low_byte;
    uint8_t *high_byte_ptr = (uint8_t *) &high_byte;
    uint8_t *low_byte_ptr = (uint8_t *) &low_byte;

    receiveBytes( high_byte_ptr, 1);
    receiveBytes( low_byte_ptr, 1);

    response->params_size = (high_byte << 8) | low_byte;
    receiveBytes( response->params, response->params_size);
    receiveBytes( (uint8_t *) &response->command_crc, 1);
    
    if (is_valid_command_crc8(payload) ) {
        sendBytes( (uint8_t *) STATUS_OK, 1);
    } else {
        sendBytes( (uint8_t *) INVALID_CRC, 1);
        return INVALID_CRC
    }

    uint8_t expected_size = response->data_size;

    receiveBytes( high_byte_ptr, 1);
    receiveBytes( low_byte_ptr, 1);
    response->data_size = (high_byte << 8) | low_byte;
    
    if data_size > expected_size {
        sendBytes( (uint8_t *) INVALID_DATA_SIZE, 1);
        return INVALID_DATA_SIZE;
    }

    receiveBytes( (uint8_t *) response->data, response->data_size);
    receiveBytes( (uint8_t *) &response->data_crc, 1);
    if (is_valid_data_crc8(payload) ) {
        sendBytes( (uint8_t *) STATUS_OK, 1);
    } else {
        sendBytes( (uint8_t *) INVALID_CRC, 1);
        return INVALID_CRC
    }
    return STATUS_OK;
}
