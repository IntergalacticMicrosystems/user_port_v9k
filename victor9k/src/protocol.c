#include fletcher16.h
#include user_port.h

void send_data_with_checksum(uint8_t *data, size_t len) {
    uint16_t sum1 = 0, sum2 = 0;

    for (size_t i = 0; i < len; i++) {
        send_byte_via_user_port(data[i]);  // Function to send a byte over the 6522
        fletcher16_byte(&sum1, &sum2, data[i]);
    }

    // At the end, send the checksum itself
    uint16_t checksum = fletcher16_finalize(sum1, sum2);
    send_byte_via_user_port(checksum & 0xFF);      // Send low byte of checksum
    send_byte_via_user_port((checksum >> 8) & 0xFF);  // Send high byte of checksum
}
