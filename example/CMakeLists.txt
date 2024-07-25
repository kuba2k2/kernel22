cmake_minimum_required(VERSION 3.24)

file(GLOB SRCS "*.c")

add_executable(Example ${SRCS})
set_target_properties(Example PROPERTIES OUTPUT_NAME "Example_${K22_BITS}")

add_executable(Example10 ${SRCS})
target_link_options(Example10 PRIVATE /SUBSYSTEM:CONSOLE,10.0)
set_target_properties(Example10 PROPERTIES OUTPUT_NAME "Example10_${K22_BITS}")
