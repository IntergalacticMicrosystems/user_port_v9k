cmake_minimum_required(VERSION 3.13)

project(user_common_library)

# Gather all source files in the common directory
file(GLOB COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Create a static library from the common sources
add_library(user_common STATIC ${COMMON_SOURCES})

# Include the current directory for the headers
target_include_directories(user_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
