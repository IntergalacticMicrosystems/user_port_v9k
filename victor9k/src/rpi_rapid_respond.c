#include <wiringPi.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>

// Define pin numbers based on WiringPi's pin numbering scheme
#define TRIGGER_PIN 3  // WiringPi 3, rpi pin 15, lvl shifter A4
#define ACK_PIN 2      // WiringPi 2, rpi pin 13, lvl shifter A3
#define LVL_SHIFTER_ENABLE_PIN 8      // WiringPi 8, pin 3
#define INPUT_PIN_COUNT 2
int inputPins[INPUT_PIN_COUNT] = {6, 7}; // Example input pins: Wpi-6 shifter A7, wpi-7 shifter A8

volatile static bool ackPulseActive = false;
struct timeval pulseStartTime;

void handle_signal(int signal) {
    // Reset ACK_PIN to a known state
    digitalWrite(ACK_PIN, HIGH); // Assuming LOW is safe; adjust as needed

    // Optionally reset other pins here...

    printf("Exiting gracefully...\n");
    exit(0); // Exit the program
}

// ISR for falling edge on TRIGGER_PIN
void triggerISR(void) {
    while (ackPulseActive) {};   // wait for previous pulse to be processed
    digitalWrite(ACK_PIN, LOW);  // Start the pulse
    ackPulseActive = true;
    gettimeofday(&pulseStartTime, NULL); 
}

int main(void) {

    signal(SIGINT, handle_signal);

    // Setup WiringPi
    if (wiringPiSetup() == -1) {
        printf("WiringPi setup failed.\n");
        return 1;
    }

    // Set up pins
    pinMode(TRIGGER_PIN, INPUT);
    pinMode(ACK_PIN, OUTPUT);
    pinMode(LVL_SHIFTER_ENABLE_PIN, OUTPUT);

    for (int i = 0; i < INPUT_PIN_COUNT; ++i) {
        pinMode(inputPins[i], INPUT);
    }

    // Disable the internal pull-up resistor on the trigger pin
    pullUpDnControl(TRIGGER_PIN, PUD_OFF);
    
    // Set ISR for falling edge detection on TRIGGER_PIN
    if (wiringPiISR(TRIGGER_PIN, INT_EDGE_FALLING , &triggerISR) < 0) {
        printf("Failed to setup ISR.\n");
        return 1;
    }

    // Set ACK_PIN to high initially
    digitalWrite(ACK_PIN, HIGH);
    digitalWrite(LVL_SHIFTER_ENABLE_PIN, HIGH);


    // Infinite loop to keep the program running
    printf("Monitoring for trigger...\n");
    while (1) {
        if (ackPulseActive) {
            struct timeval now;
            gettimeofday(&now, NULL);
            long elapsedTime = (now.tv_sec - pulseStartTime.tv_sec) * 1000000; // sec to us
            elapsedTime += (now.tv_usec - pulseStartTime.tv_usec); // add us to us
            if (elapsedTime > 1) { // Check if 10 microseconds have passed
                digitalWrite(ACK_PIN, HIGH); // End the pulse
                ackPulseActive = false; // Reset the flag
                printf("Moving ACK pin high again...\n");
            }
        }
    }

    return 0;
}
