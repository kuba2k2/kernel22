cmake_minimum_required(VERSION 3.24)

add_library(K22Common INTERFACE)
target_include_directories(K22Common INTERFACE "include/")
target_compile_definitions(K22Common INTERFACE "K22_BITS${K22_BITS}")
target_link_libraries(K22Common INTERFACE uthash)

add_subdirectory("core/")
add_subdirectory("loader/")
add_subdirectory("patcher/")
add_subdirectory("verifier/")

set_target_properties(
	K22Core K22Loader K22Patcher K22Verifier PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
