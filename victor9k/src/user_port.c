#include "./user_port.h"

static bool viaInitialized = false;
static volatile bool sendAcknowledged = false;
static volatile bool dataAvailable = false;
static volatile bool protocolTimeout = false;
extern bool debug;
static void (__interrupt __far *originalISR)();

static volatile V9kParallelPort far *via1 = MK_FP(PHASE2_DEVICE_SEGMENT,
      VIA1_REG_OFFSET);
static volatile V9kParallelPort far *via2 = MK_FP(PHASE2_DEVICE_SEGMENT,
      VIA2_REG_OFFSET);
static volatile V9kParallelPort far *via3 = MK_FP(PHASE2_DEVICE_SEGMENT,
      VIA3_REG_OFFSET);
static volatile uint8_t far *pic = MK_FP(INTEL_DEV_SEGMENT, PIC_8259_OFFSET);

void outPortByte(volatile uint8_t far* port, uint8_t value) {
   printf("outportbyte: port %x, value %x\n", (void*)via1, value);
   if (! viaInitialized) {
      printf("outportbyte: VIA NOT INITIALIZED, calling par_port_init()\n");
      userPortInitialize();
   }
   //Disable();  /* Disable interrupts */
   via1->out_in_reg_a = value;
   //Enable();   /* Enable interrupts */
   //*port = value;
   return;
}

uint8_t inPortByte(volatile uint8_t far* port) {
   uint8_t data;
   //Disable();  /* Disable interrupts */
   data = via1->out_in_reg_b;
   //Enable();   /* Enable interrupts */
   printf("inportbyte: data: %x\n", data);
   return data;
}

void receiveBytesPA(uint8_t* data, size_t length) {
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

send_byte_via_user_port(uint8_t byte) {
    void sendBytesPB(uint8_t* data, size_t length)
}
 

void sendBytesPB(uint8_t* data, size_t length) {
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

void sendBytesViaShiftRegister(const uint8_t* data, size_t length) {
   // Configure the shift register for output (free-run mode)
   via3->aux_ctrl_reg = (via3->aux_ctrl_reg & SR_RESET_MODE) | SHIFT_OUT_MODE_110;
   via3->int_enable_reg = SR_INTERRUPT_MASK | INTERRUPT_ENABLE; // Enable shift register interrupts

   printf("sendBytesViaShiftRegister start\n");

   for (size_t i = 0; i < length; ++i) {
      printf("%d  ", i);
      sendAcknowledged = false;
      via3->shift_reg = data[i]; // Load the data into the shift register

      // Wait for the data to be clocked out
      while (!sendAcknowledged) { }
   }
   printf("sendBytesViaShiftRegister end\n");
}

void interrupt far userPortISR(void) {
   // Check if the interrupt is for your device and handle it
   if (via3->int_flag_reg > 0) {
      via3->int_flag_reg = VIA_CLEAR_INTERRUPTS; // Clear all interrupt flags
      *pic = PIC_VIA_SPECIFIC_EOI; // Send End of Interrupt command to the PIC command register
      sendAcknowledged = true;
      _chain_intr(originalISR);         //send the interrupt up the chain for other 6522's
   } else {
      // Call the original ISR
      _chain_intr(originalISR);
   }
}

void userPortInitialize(void) {
   //Via-1 is main parallel port
   printf("Address of via3: %x\n", (void*)via3);
   printf("via3->out_in_reg_a: %x\n", (void*)via3);
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
   return;
}

int main() {
   userPortInitialize();

   // Save the original 6522 Interrupt Service Routine [ISR] address
   originalISR = _dos_getvect(INTERRUPT_NUM);

   // Set your ISR
   _dos_setvect( INTERRUPT_NUM, userPortISR );

   uint8_t sendData[200]; // Example sendData to send
   for (int i = 0; i < 200; ++i) {
      sendData[i] = i; // Initialize sendData
   }

   clock_t start_time, end_time;

   // start_time = clock();
   // sendBytesViaShiftRegister(sendData, 200);
   // end_time = clock();
   // printf( "sendBytesViaShiftRegister Execution time was %lu seconds\n",
   //        (end_time - start_time) / CLOCKS_PER_SEC );

   uint8_t receiveData[200]; // Example buffer to receive data
   start_time = clock();
   receiveBytesPA(receiveData, 200);
   end_time = clock();
   printf( "receiveBytesPA Execution time was %lu seconds\n",
           (end_time - start_time) / CLOCKS_PER_SEC );
   printf("Data Received: %s", receiveData);

   start_time = clock();
   sendBytesPB(sendData, 200);
   end_time = clock();
   printf( "sendBytesPB Execution time was %lu seconds\n",
           (end_time - start_time) / CLOCKS_PER_SEC );

   

   // Before exiting, restore the original ISR
   _dos_setvect(INTERRUPT_NUM, originalISR);

   return 0;
}





