// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#pragma once

// Loglevels
#define K22_LEVEL_TRACE	  0
#define K22_LEVEL_VERBOSE 1
#define K22_LEVEL_DEBUG	  2
#define K22_LEVEL_INFO	  3
#define K22_LEVEL_WARN	  4
#define K22_LEVEL_ERROR	  5
#define K22_LEVEL_FATAL	  6
#define K22_LEVEL_NONE	  7

// Logger format options
#ifndef K22_LOGGER_TIMESTAMP
#define K22_LOGGER_TIMESTAMP 1
#endif

#ifndef K22_LOGGER_FILE
#define K22_LOGGER_FILE 1
#endif

#ifndef K22_LOGGER_FUNCTION
#define K22_LOGGER_FUNCTION 0
#endif

#ifndef K22_LOGGER_COLOR
#define K22_LOGGER_COLOR 0
#endif

// Logger options
#ifndef K22_LOGLEVEL
#define K22_LOGLEVEL K22_LEVEL_DEBUG
#endif

#ifndef K22_LOG_OUTPUT_DEBUG_STRING
#define K22_LOG_OUTPUT_DEBUG_STRING 1
#endif

// Loader configuration
#ifndef K22_LOADER_DEBUGGER
#define K22_LOADER_DEBUGGER 1
#endif

// Core configuration
#ifndef K22_REG_KEY_PATH
#define K22_REG_KEY_PATH "SOFTWARE\\kuba2k2\\Kernel22"
#endif
