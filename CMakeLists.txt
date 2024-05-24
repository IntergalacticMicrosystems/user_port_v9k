cmake_minimum_required(VERSION 3.13)

project(MyProject)
SET(APPLE FALSE)
set(CMAKE_SYSTEM_NAME Linux) 
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Include subdirectories
add_subdirectory(common)
add_subdirectory(pico)
add_subdirectory(victor9k)
