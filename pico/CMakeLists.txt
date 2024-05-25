# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
SET(APPLE FALSE)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)


# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
   set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(PICO_SDK_PATH ${USERHOME}/.pico-sdk/sdk/1.5.1)
set(PICO_TOOLCHAIN_PATH ${USERHOME}/.pico-sdk/toolchain/13_2_Rel1)
if(WIN32)
    set(pico-sdk-tools_DIR ${USERHOME}/.pico-sdk/tools/1.5.1)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config.cmake)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config-version.cmake)
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(user_port_pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

if (NOT TARGET no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)
  add_subdirectory(../sdio-fatfs/src build)
endif() 


# Add executable. Default name is the project name, version 0.1

add_executable(user_port_pico user_port_pico.c )

pico_set_program_name(user_port_pico "user_port_pico")
pico_set_program_version(user_port_pico "0.1")

# Generate PIO header
pico_generate_pio_header(user_port_pico ${CMAKE_CURRENT_LIST_DIR}/receive_fifo.pio)
pico_generate_pio_header(user_port_pico ${CMAKE_CURRENT_LIST_DIR}/transmit_fifo.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(user_port_pico 1)
pico_enable_stdio_usb(user_port_pico 0)

# Add the standard library to the build
target_link_libraries(user_port_pico
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(user_port_pico PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(
        user_port_pico 
        pico_multicore
        pico_stdlib
        hardware_spi
        hardware_pio
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
        )

pico_add_extra_outputs(user_port_pico)

