# pico_toolchain.cmake
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
SET(CMAKE_C_COMPILER /Applications/ArmGNUToolchain/13.2.Rel1/arm-none-eabi/bin/arm-none-eabi-gcc)
SET(CMAKE_CXX_COMPILER /Applications/ArmGNUToolchain/13.2.Rel1/arm-none-eabi/bin/arm-none-eabi-g++)

# Set compiler and linker flags
SET(CMAKE_C_FLAGS "-mcpu=cortex-m0plus -mthumb")
SET(CMAKE_CXX_FLAGS "-mcpu=cortex-m0plus -mthumb")
SET(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m0plus -mthumb -nostartfiles -lc -lgcc -lnosys")

# Ensure the linker uses the appropriate libraries
SET(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Override the default CMake compiler check behavior
SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER_WORKS 1)