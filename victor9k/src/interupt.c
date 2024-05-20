#include <dos.h>
#include <stdint.h>
#include <stdio.h>

// small code snippet to demonstrate writing a DOS interrupt and how to handle
// Assume INTERRUPT_NUM is the interrupt number you're sharing
#define INTERRUPT_NUM 0x43 
#define IVT_ENTRY (INTERRUPT_NUM * 4) // Each Interrupt Vector Table [IVT] entry is 4 bytes

// Structure for an interrupt vector (far pointer)
typedef struct {
    uint16_t offset;
    uint16_t segment;
} INTERRUPT_VECTOR;

// Global variable to store the original ISR address
void (interrupt far *originalISR)();

// Function prototypes
void interrupt far userPortISR(void);
void (__interrupt __far *originalISR)();
void setIVTEntry(int intNum, void interrupt (*isr)(void));

int main() {
    // Save the original ISR address
    originalISR = _dos_getvect(INTERRUPT_NUM);

    // Set your ISR
    _dos_setvect( INTERRUPT_NUM, userPortISR );

    // Your program code here

    // Before exiting, restore the original ISR
    _dos_setvect(INTERRUPT_NUM, originalISR);

    return 0;
}

void interrupt far userPortISR(void) {
    // Check if the interrupt is for your device and handle it
    if (/* interrupt is for your device */) {
        // Handle the interrupt

        // Acknowledge the interrupt at the device level if necessary
    } else {
        // Call the original ISR
        _chain_intr((void interrupt (*)(void))MK_FP(originalISR.segment, originalISR.offset));
    }
}

void interrupt (*getIVTEntry(int intNum))(void) {
    // Returns the ISR at intNum
    return (void interrupt (*)(void)) *(INTERRUPT_VECTOR far *)MK_FP(0, intNum * 4);
}


