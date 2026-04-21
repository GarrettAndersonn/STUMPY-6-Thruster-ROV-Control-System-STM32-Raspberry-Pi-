cmake_minimum_required(VERSION 3.22)

project(STUMPY)

# Top-level project aggregator.
# The STM32 firmware project is located in ./firmware
# Python tools are located in ./raspberry_pi and ./laptop

add_subdirectory(firmware)
