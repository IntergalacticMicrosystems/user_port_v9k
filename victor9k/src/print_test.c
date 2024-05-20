#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "cprint.h"  // Include the header file where writeToDriveLog is declared
#include "device.h" 
#include "template.h"


// Test function
void test_writeToDriveLog() {
    MiniDrive testDrive = {0};  // Initialize a test MiniDrive

    cprintf("SD: test_writeToDriveLog *drive = %x:%x\n", FP_SEG(testDrive), FP_OFF(testDrive));

    // Call writeToDriveLog with test data
    writeToDriveLog(&testDrive, "Test %d, %s, %x", 123, "string", 0xABCD);

    // Verify the result
    char expectedOutput[SECTOR_SIZE];
    sprintf(expectedOutput, "Test 123, string, ABCD");
    assert(strcmp((char*)testDrive.sectors + 5 * SECTOR_SIZE, expectedOutput) == 0);

    // Add more test cases as needed
}

int main() {
    switch_stack();
    push_regs();
    test_writeToDriveLog();
    // Add more test functions if you have them
    pop_regs();
    restore_stack();
    return 0;
}
