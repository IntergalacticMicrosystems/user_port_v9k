#include <stdint.h>

// Assuming these are the correct memory-mapped addresses for the NEC 7201
#define BAUD_RATE_CONTROL_REGISTER ((volatile uint8_t*)0xE002)
#define DATA_CONTROL_REGISTER ((volatile uint8_t*)0xE004)

//Transmission Speed: x1, x16, x32 or x64 Clock Frequency
// Baud rate conversion table
uint8_t rates[] = {0x00, 0x01, 0x80, 0x00, 0x40, 0x00, 0x20, 0x00,
                   0x10, 0x00, 0x08, 0x00, 0x04, 0x00};

void BAUD_SET(uint8_t port, uint8_t rateIndex) {
    // Pointer to the baud rate control register
    volatile uint8_t* controlReg = BAUD_RATE_CONTROL_REGISTER;
    // Adjust pointer for port selection
    controlReg += (port == 0) ? 3 : 0; // Assuming port A is at +3 offset, adjust if necessary
    
    // Set baud rate for port A or B
    if (port > 0) {
        // Setting for Port B
        *controlReg = 0x76; // Assuming this is correct for initiating baud rate set for Port B
    } else {
        // Setting for Port A
        *controlReg = 0x36; // Assuming this is correct for initiating baud rate set for Port A
    }

    // Calculate baud rate setting address
    uint16_t baudRateSetting = *(uint16_t*)&rates[rateIndex * 2];
    
    // Send baud rate setting to the control register
    *(controlReg++) = (uint8_t)(baudRateSetting & 0xFF); // Send low byte
    *controlReg = (uint8_t)(baudRateSetting >> 8); // Send high byte
}

void SEND_CHAR(uint8_t port, uint8_t character) {
    volatile uint8_t* dataReg = DATA_CONTROL_REGISTER + 2 + (port * 2); // Adjust for correct port and status/data register
    while (!(*dataReg & 0x04)); // Wait for TX buffer to be empty
    *(dataReg - 2) = character; // Send character
}

uint8_t GET_CHAR(uint8_t port) {
    volatile uint8_t* dataReg = DATA_CONTROL_REGISTER + 2 + (port * 2); // Adjust for correct port and status/data register
    while (!(*dataReg & 0x01)); // Wait for RX buffer to have data
    return *(dataReg - 2); // Read character
}
