cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(namebadge C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(namebadge)

pico_generate_pio_header(namebadge ${CMAKE_CURRENT_LIST_DIR}/src/driver.pio)

target_sources(namebadge
        PRIVATE
        src/namebadge.c
)

pico_enable_stdio_usb(namebadge 0)
pico_enable_stdio_uart(namebadge 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(namebadge)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(namebadge
        PRIVATE
        pico_stdlib
        hardware_pio
)
