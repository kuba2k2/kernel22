cmake_minimum_required(VERSION 3.25)
project(dlls)

macro(add_dll name)
	file(GLOB SRCS "${name}/*.c")
	add_library("DLL_${name}" SHARED ${SRCS})
	target_link_libraries("DLL_${name}" PRIVATE core)
#	target_link_options("DLL_${name}" PRIVATE /nodefaultlib /entry:DllMain)
#	set_target_properties("DLL_${name}" PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_target_properties("DLL_${name}" PROPERTIES OUTPUT_NAME "${name}")
	set_target_properties("DLL_${name}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DLL_${K22_BITS}")
endmacro()

add_dll(kernel32)
