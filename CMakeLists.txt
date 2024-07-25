cmake_minimum_required(VERSION 3.24)

project(kernel)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(K22_BITS "64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(K22_BITS "32")
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/K22_${K22_BITS}")

add_link_options(/INCREMENTAL:NO)
link_libraries(ntdll kernel32 version)

add_subdirectory("lib/")
add_subdirectory("src/")
add_subdirectory("example/")
add_subdirectory("dlls/")

function(print_all_targets DIR)
	get_property(TARGETS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
	foreach (TARGET IN LISTS TARGETS)
		string(REPLACE "${CMAKE_CURRENT_LIST_DIR}" "" RELDIR "${DIR}")
		message(STATUS "Target: ${TARGET} | .${RELDIR}/")
	endforeach ()

	get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
	foreach (SUBDIR IN LISTS SUBDIRS)
		print_all_targets("${SUBDIR}")
	endforeach ()
endfunction()

print_all_targets("${CMAKE_CURRENT_LIST_DIR}")
