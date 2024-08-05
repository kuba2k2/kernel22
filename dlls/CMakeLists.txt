cmake_minimum_required(VERSION 3.24)

macro(add_dll NAME)
	file(GLOB SRCS "${NAME}/*.c")
	add_library("Dll${NAME}" SHARED ${SRCS})
	target_link_libraries("Dll${NAME}" PRIVATE K22Core)
	#	target_link_options("Dll${NAME}" PRIVATE /nodefaultlib /entry:DllMain)
	#	set_target_properties("Dll${NAME}" PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set_target_properties("Dll${NAME}" PROPERTIES OUTPUT_NAME "Dll${NAME}")
	set_target_properties("Dll${NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DLL_${K22_BITS}")
endmacro()

add_dll(Kernel32)
add_dll(WinVer)
