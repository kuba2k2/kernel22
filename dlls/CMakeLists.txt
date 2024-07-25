cmake_minimum_required(VERSION 3.24)

macro(add_dll NAME)
	file(GLOB SRCS "${NAME}/*.c")
	add_library("DLL_${NAME}" SHARED ${SRCS})
	target_link_libraries("DLL_${NAME}" PRIVATE K22Core)
	#	target_link_options("DLL_${NAME}" PRIVATE /nodefaultlib /entry:DllMain)
	#	set_target_properties("DLL_${NAME}" PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_target_properties("DLL_${NAME}" PROPERTIES OUTPUT_NAME "${NAME}")
	set_target_properties("DLL_${NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DLL_${K22_BITS}")
endmacro()

add_dll(kernel32)
