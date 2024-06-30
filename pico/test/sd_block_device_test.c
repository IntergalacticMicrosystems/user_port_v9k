#include "../Unity/src/unity.h"
#include "../include/sd_block_device.h"
#include "mock_sd_card.h"
#include "../common/dos_device_payloads.h"
#include "../common/protocols.h"
#include "../common/crc8.h"

SDState *sdState;
PIO_state pioState;
Payload payload;

// Define the mock SDState with 2 files
static SDState mockSDState = {
    .imgFiles = {
        "0_pc_testfile.img",
        "1_v9k_testfile.img"
    },
    .fileCount = 2
};

// Define the mock BPB
static const BPB basic_bpb = {
    .bytes_per_sector = 512,
    .sectors_per_cluster = 4,
    .reserved_sectors = 1,
    .num_fats = 2,
    .root_entry_count = 512,
    .total_sectors = 20480,
    .media_descriptor = 0xF8,
    .sectors_per_fat = 40,
    .sectors_per_track = 32,
    .num_heads = 64,
    .hidden_sectors = 0
};

// VictorCommand for BUILD_BPB
static VictorCommand build_bpb_command = {
    .protocol = SD_BLOCK_DEVICE,
    .byte_count = sizeof(BPB) + 2,
    .command = BUILD_BPB,
    .params = (uint8_t*)&basic_bpb,
    .expected_crc = 0x00
};


void setUp(void) {
    sdState = initializeSDState("/test_directory");
}

void tearDown(void) {
    // Clean up function, runs after each test
}

void test_crc8(void) {
    createCommandCrc8(&build_bpb_command);
    TEST_ASSERT_TRUE(isValidCommandCrc8(&build_bpb_command));
}

void test_initializeSDState(void) {
    SDState *state = initializeSDState("/test_directory");
    TEST_ASSERT_NOT_NULL(state);
}

void test_initSDCard(void) {
    bool result = initSDCard(sdState, &pioState, &payload);
    TEST_ASSERT_TRUE(result);
}

void test_mediaCheck(void) {
    bool result = mediaCheck(sdState, &pioState, &payload);
    TEST_ASSERT_TRUE(result);
}

void test_buildBpb(void) {
    bool result = buildBpb(sdState, &pioState, &payload);
    TEST_ASSERT_TRUE(result);
}

void test_victor9kDriveInfo(void) {
    bool result = victor9kDriveInfo(sdState, &pioState, &payload);
    TEST_ASSERT_TRUE(result);
}

void test_sdRead(void) {
    uint8_t buffer[512];
    int result = sdRead(sdState, &pioState, &payload);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_sdWrite(void) {
    uint8_t buffer[512] = {0};
    int result = sdWrite(sdState, &pioState, &payload);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_send_error_response(void) {
    bool result = send_error_response(sdState, &pioState, &payload);
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_initializeSDState);
    RUN_TEST(test_initSDCard);
    RUN_TEST(test_mediaCheck);
    RUN_TEST(test_buildBpb);
    RUN_TEST(test_victor9kDriveInfo);
    RUN_TEST(test_sdRead);
    RUN_TEST(test_sdWrite);
    RUN_TEST(test_send_error_response);
    return UNITY_END();
}