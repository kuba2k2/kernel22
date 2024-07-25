cmake_minimum_required(VERSION 3.24)

file(GLOB SRCS "*.c" "../common/*.c")

add_library(K22Core SHARED ${SRCS})
target_link_libraries(K22Core PUBLIC K22Common)
target_link_libraries(K22Core PRIVATE minhook)
target_include_directories(K22Core PRIVATE ".")
target_compile_definitions(K22Core PRIVATE K22_CORE=1)
set_target_properties(K22Core PROPERTIES OUTPUT_NAME "K22Core")
