cmake_minimum_required(VERSION 3.25)
project(kernel)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(K22_BITS "64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(K22_BITS "32")
endif ()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/K22_${K22_BITS}")

file(GLOB SRCS_CORE "src/common/*.c" "src/core/*.c")
file(GLOB SRCS_LOADER "src/loader/*.c")
file(GLOB SRCS_PATCHER "src/patcher/*.c")
file(GLOB SRCS_VERIFIER "src/verifier/*.c" "src/common/k22_import_table.c")
file(GLOB SRCS_EXAMPLE "example/*.c")

add_link_options(/INCREMENTAL:NO)
link_libraries(ntdll kernel32 version)
include_directories("src/" "src/common/")
add_compile_definitions("K22_BITS${K22_BITS}")

add_library(core SHARED ${SRCS_CORE})
target_include_directories(core PRIVATE "src/core/")
target_compile_definitions(core PRIVATE K22_CORE=1)
set_target_properties(core PROPERTIES OUTPUT_NAME "K22Core")

add_executable(loader ${SRCS_LOADER})
target_link_libraries(loader PRIVATE core)
target_include_directories(loader PRIVATE "src/loader/")
target_compile_definitions(loader PRIVATE K22_LOADER=1)
set_target_properties(loader PROPERTIES OUTPUT_NAME "K22Loader")

add_executable(patcher ${SRCS_PATCHER})
target_link_libraries(patcher PRIVATE core)
target_include_directories(patcher PRIVATE "src/patcher/")
target_compile_definitions(patcher PRIVATE K22_PATCHER=1)
set_target_properties(patcher PROPERTIES OUTPUT_NAME "K22Patcher")

add_library(verifier SHARED ${SRCS_VERIFIER})
target_link_options(verifier PRIVATE /nodefaultlib /entry:DllMain)
target_include_directories(verifier PRIVATE "src/verifier/")
target_compile_definitions(verifier PRIVATE K22_VERIFIER=1)
set_target_properties(verifier PROPERTIES OUTPUT_NAME "K22Verifier")

add_executable(example ${SRCS_EXAMPLE})
set_target_properties(example PROPERTIES OUTPUT_NAME "Example${K22_BITS}")

set_target_properties(
	core loader patcher verifier PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
