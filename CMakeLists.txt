cmake_minimum_required(VERSION 3.25)
project(kernel)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(K22_BITS "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(K22_BITS "32")
endif()

set(CMAKE_CXX_STANDARD 17)

file(GLOB SRCS_CORE "src/*.c" "src/core/*.c")
file(GLOB SRCS_LOADER "src/*.c" "src/loader/*.c")
file(GLOB SRCS_EXAMPLE "example/*.c")

add_link_options(/INCREMENTAL:NO)
link_libraries(ntdll kernel32 version)
include_directories("src/")

add_library(core SHARED ${SRCS_CORE})
target_include_directories(core PUBLIC "src/core/")
target_compile_definitions(core PUBLIC K22_CORE=1)
set_target_properties(core PROPERTIES OUTPUT_NAME "K22Core${K22_BITS}")

add_executable(loader ${SRCS_LOADER})
target_link_libraries(loader PUBLIC core)
target_include_directories(loader PUBLIC "src/loader/")
target_compile_definitions(loader PUBLIC K22_LOADER=1)
set_target_properties(loader PROPERTIES OUTPUT_NAME "K22Loader${K22_BITS}")

add_executable(example ${SRCS_EXAMPLE})

set_target_properties(
	core loader PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
