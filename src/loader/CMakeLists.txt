cmake_minimum_required(VERSION 3.24)

file(GLOB SRCS "*.c")

add_executable(K22Loader ${SRCS})
target_link_libraries(K22Loader PRIVATE K22Core)
target_include_directories(K22Loader PRIVATE ".")
target_compile_definitions(K22Loader PRIVATE K22_LOADER=1)
set_target_properties(K22Loader PROPERTIES OUTPUT_NAME "K22Loader")
