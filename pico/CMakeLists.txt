# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
SET(APPLE FALSE)
set(CMAKE_SYSTEM_NAME Linux)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
set(PICO_BOARD pico CACHE STRING "Board type")

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
   set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(PICO_SDK_PATH ${USERHOME}/projects/pico-sdk)
set(PICO_TOOLCHAIN_PATH /Applications/ArmGNUToolchain/13.2.Rel1/arm-none-eabi/bin/)
if(WIN32)
    set(pico-sdk-tools_DIR ${USERHOME}/.pico-sdk/tools/1.5.1)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config.cmake)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config-version.cmake)
endif()
# ====================================================================================

set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)
set(CMAKE_C_COMPILER /Applications/ArmGNUToolchain/13.2.Rel1/arm-none-eabi/bin/arm-none-eabi-gcc --specs=nano.specs -mfloat-abi=soft -mtune=cortex-m0plus -mcpu=cortex-m0plus)
set(CMAKE_CXX_COMPILER /Applications/ArmGNUToolchain/13.2.Rel1/arm-none-eabi/bin/arm-none-eabi-g++ --specs=nano.specs -mfloat-abi=soft -mtune=cortex-m0plus -mcpu=cortex-m0plus)

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES 
   /Applications/ArmGNUToolchain/13.2.Rel1/arm-none-eabi/arm-none-eabi/include
    )

project(user_port_pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()



# Add executable. Default name is the project name, version 0.1
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../common user_common)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sdio-fatfs/src sdio-fatfs)
add_subdirectory(lib)
add_subdirectory(src)

enable_testing()
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../Unity Unity)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test test)





