cmake_minimum_required(VERSION 3.25)
project(kernel)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SRCS_LOADER src/*.c src/loader/*.c)
file(GLOB SRCS_MODULE src/*.c src/module/*.c)
file(GLOB SRCS_EXAMPLE example/*.c)

add_link_options(/INCREMENTAL:NO)

add_executable(loader ${SRCS_LOADER})
target_include_directories(
	loader PUBLIC
	"src/"
	"src/loader/"
)
target_link_options(
	loader PUBLIC
	/NODEFAULTLIB:MSVCRT
	"ntdll.lib"
)

add_library(module SHARED ${SRCS_MODULE})
target_include_directories(
	module PUBLIC
	"src/"
	"src/module/"
)
target_link_options(
	module PUBLIC
	/NODEFAULTLIB:MSVCRT
	"ntdll.lib"
)

set_property(TARGET loader PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_options(loader PUBLIC /D K22_LOADER=1)

set_property(TARGET module PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_options(module PUBLIC /D K22_MODULE=1)

add_executable(example ${SRCS_EXAMPLE})
target_link_options(
	example PUBLIC
	version.lib
)
