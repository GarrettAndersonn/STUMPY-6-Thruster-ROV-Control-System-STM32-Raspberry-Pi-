cmake_minimum_required(VERSION 3.22)

project(STUMPY_FIRMWARE C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# This CMake file is included mainly to document the firmware structure.
# The project is intended to be opened and built primarily with STM32CubeIDE.
#
# For a full standalone CMake build, the following must be present and valid:
# - Drivers/
# - Core/
# - startup_stm32f446retx.s
# - STM32F446RETX_FLASH.ld
# - CMSIS / HAL include paths
#
# Adjust these paths if your local firmware structure changes.

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_compile_options(
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
)

add_link_options(
    ${MCU_FLAGS}
    -Wl,--gc-sections
)

add_executable(${PROJECT_NAME}.elf
    Core/Src/main.c
    Core/Src/stm32f4xx_it.c
    Core/Src/stm32f4xx_hal_msp.c
    Core/Src/system_stm32f4xx.c
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    USE_HAL_DRIVER
    STM32F446xx
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    Core/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
)

# Uncomment when using a full standalone CMake build with startup + linker files:
#
# target_sources(${PROJECT_NAME}.elf PRIVATE startup_stm32f446retx.s)
# target_link_options(${PROJECT_NAME}.elf PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F446RETX_FLASH.ld)
