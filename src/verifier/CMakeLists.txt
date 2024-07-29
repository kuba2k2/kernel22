cmake_minimum_required(VERSION 3.24)

file(GLOB SRCS "*.c" "../common/k22_patch.c" "../common/k22_patch_impl.c")

add_library(K22Verifier SHARED ${SRCS})
target_link_options(K22Verifier PRIVATE /nodefaultlib /entry:DllMain)
target_link_libraries(K22Verifier PRIVATE K22Common)
target_include_directories(K22Verifier PRIVATE ".")
target_compile_definitions(K22Verifier PRIVATE K22_VERIFIER=1)
set_target_properties(K22Verifier PROPERTIES OUTPUT_NAME "K22Verifier")
