# src/CMakeLists.txt
cmake_minimum_required(VERSION 3.13)

add_executable(user_port_pico user_port_pico.c )

pico_set_program_name(user_port_pico "user_port_pico")
pico_set_program_version(user_port_pico "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(user_port_pico 1)
pico_enable_stdio_usb(user_port_pico 0)

# Add any user requested libraries
target_link_libraries(
        user_port_pico 
        user_port_lib
        user_common_lib
        pico_multicore
        pico_stdlib
        hardware_spi
        hardware_pio
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
        )

pico_add_extra_outputs(user_port_pico)