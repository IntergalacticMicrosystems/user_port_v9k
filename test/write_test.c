#include "../Unity/unity.h"

// Test SD Write Success
void test_sd_write_success(void) {
    uint8_t buffer[512] = {0};  // Test data
    TEST_ASSERT_EQUAL_INT(0, sd_write(0, 0, buffer, 1));  // Write 1 block to block 0
}

// Test SD Write Error Handling
void test_sd_write_failure(void) {
    uint8_t buffer[512] = {1};  // Test data
    TEST_ASSERT_EQUAL_INT(-1, sd_write(0, 999999, buffer, 1));  // Try to write to an invalid block
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sd_write_success);
    RUN_TEST(test_sd_write_failure);
    return UNITY_END();
}

