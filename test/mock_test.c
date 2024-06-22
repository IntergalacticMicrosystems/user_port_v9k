#include "unity.h"
#include "mock_sd_card.h"

void setUp(void) {
    // Initialize mock before each test
    mock_sd_initialize(0, 0);
}

void tearDown(void) {
    // Clean up function, runs after each test
}

void test_sd_read_success(void) {
    uint8_t buffer[512];
    TEST_ASSERT_EQUAL_INT(0, mock_sd_read(0, 0, buffer, 1));
}

void test_sd_write_success(void) {
    uint8_t data[512] = {0};
    TEST_ASSERT_EQUAL_INT(0, mock_sd_write(0, 0, data, 1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sd_read_success);
    RUN_TEST(test_sd_write_success);
    return UNITY_END();
}

