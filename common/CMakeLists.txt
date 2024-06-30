cmake_minimum_required(VERSION 3.13)

project(user_common_library)

add_library(user_common_lib STATIC
    protocols.h
    dos_device_payloads.h
    crc8.h    
    crc8.c
    fletcher.c
)

# Include the current directory for the headers
target_include_directories(user_common_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
