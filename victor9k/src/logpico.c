#include <dos.h>
#include <stdint.h>
#include <i86.h> 
#include <string.h> 

#include "cprint.h"     /* Console printing direct to hardware */
#include "v9_communication.h"  /* Victor 9000 communication protocol */
#include "../../common/protocols.h"
#include "../../common/dos_device_payloads.h"
#include "../../common/crc8.h"

uint16_t log_message (char *message, uint16_t message_size) {
 
  //Prepare satic payload Params
    Payload logPayload = {0};
    logPayload.protocol = LOG_OUTPUT;
    logPayload.command = LOG_OUTPUT;

    logPayload.params_size = 1;
    uint8_t params[1] = {0};
    logPayload.params = &params[0];
    logPayload.data_size = message_size;
    logPayload.data = message;
    
    //cdprintf("logging data_size: %u\n", (uint16_t) logPayload.data_size);
    create_payload_crc8(&logPayload);

    ResponseStatus outcome = send_command_payload(&logPayload);
    if (outcome != STATUS_OK) {
        cdprintf("Error: Failed to send LOG_OUTPUT command to pico. Outcome: %u\n", (uint16_t) outcome);
        return 1;
    } 
  
    //getting the response from the pico
    //cdprintf("command sent success, starting receive response\n");

    Payload responsePayload = {0};
    uint8_t response_params[1] = {0};
    responsePayload.params = &response_params[0];
    responsePayload.data = &response_params[0];
    responsePayload.data_size = 1;
    outcome = receive_response(&responsePayload);
    if (outcome != STATUS_OK) {
        cdprintf("SD Error: Failed to receive response from LOG_OUTPUT %u\n", (uint16_t) outcome);
        return 1;
    }

  return 0;
}
