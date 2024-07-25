cmake_minimum_required(VERSION 3.24)

file(GLOB SRCS "*.c")

add_executable(K22Patcher ${SRCS})
target_link_libraries(K22Patcher PRIVATE K22Core)
target_include_directories(K22Patcher PRIVATE ".")
target_compile_definitions(K22Patcher PRIVATE K22_PATCHER=1)
set_target_properties(K22Patcher PROPERTIES OUTPUT_NAME "K22Patcher")
