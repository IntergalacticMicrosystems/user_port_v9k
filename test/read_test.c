#include "../Unity/src/unity.h"
#include "../pico/sd_block_device.h"

void setUp(void) {
    // Set up function, runs before each test
    sd_initialize(0, 0);  // Assuming unit and partno are zero-indexed
}

void tearDown(void) {
    // Clean up function, runs after each test
}

// Test Initialization
void test_sd_initialize(void) {
    TEST_ASSERT_TRUE(sd_initialize(0, 0));
}

// Test SD Read Success
void test_sd_read_success(void) {
    uint8_t buffer[512];  // Assuming 512-byte sectors
    TEST_ASSERT_EQUAL_INT(0, sd_read(0, 0, buffer, 1)); // Read 1 block from block 0
    // Optionally, check buffer content if there's a way to know expected data
}

// Test SD Read Error Handling
void test_sd_read_failure(void) {
    uint8_t buffer[512];
    TEST_ASSERT_EQUAL_INT(-1, sd_read(0, 999999, buffer, 1)); // Assuming block 999999 is invalid
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sd_initialize);
    RUN_TEST(test_sd_read_success);
    RUN_TEST(test_sd_read_failure);
    return UNITY_END();
}

