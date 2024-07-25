cmake_minimum_required(VERSION 3.24)

add_subdirectory("minhook/")

add_library(uthash INTERFACE)
target_include_directories(uthash INTERFACE "uthash/src/")
