cmake_minimum_required(VERSION 3.13)

project(user_port_victor9k)

SET(APPLE FALSE)

# Custom target to invoke wmake
add_custom_command(
    OUTPUT "src/userport.sys"
    COMMAND ${CMAKE_SOURCE_DIR}/compile.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Victor 9000 driver using wmake"
    VERBATIM
)