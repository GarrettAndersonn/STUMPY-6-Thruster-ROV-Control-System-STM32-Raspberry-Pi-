cmake_minimum_required(VERSION 3.22)

project(STUMPY)

# Top-level project aggregator.
# Firmware is built primarily with STM32CubeIDE.
# Python utilities are run directly.

add_subdirectory(firmware)
