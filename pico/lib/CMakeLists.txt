cmake_minimum_required(VERSION 3.13)

add_library(user_port_lib STATIC
    log_functions.c
    command_dispatch.c
    v9k_hard_drives.c
    pico_communication.c
    sd_block_device.c
)


# Generate PIO header
pico_generate_pio_header(user_port_lib ${CMAKE_CURRENT_LIST_DIR}/receive_fifo.pio)
pico_generate_pio_header(user_port_lib ${CMAKE_CURRENT_LIST_DIR}/transmit_fifo.pio)

target_link_libraries(user_port_lib 
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_pio
    user_common_lib
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    )

target_include_directories(user_port_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../include
)