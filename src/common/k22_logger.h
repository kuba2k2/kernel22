// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#pragma once

#include "kernel22.h"

#if K22_LOGGER_FILE && K22_LOGGER_FUNCTION
#define K22_LOG(dwLevel, lpFile, dwLine, lpFunction, bWin32Error, ...)                                                 \
	K22LogWrite(dwLevel, lpFile, dwLine, lpFunction, bWin32Error, __VA_ARGS__)
#elif K22_LOGGER_FILE
#define K22_LOG(dwLevel, lpFile, dwLine, lpFunction, bWin32Error, ...)                                                 \
	K22LogWrite(dwLevel, lpFile, dwLine, NULL, bWin32Error, __VA_ARGS__)
#elif K22_LOGGER_FUNCTION
#define K22_LOG(dwLevel, lpFile, dwLine, lpFunction, bWin32Error, ...)                                                 \
	K22LogWrite(dwLevel, NULL, 0, lpFunction, bWin32Error, __VA_ARGS__)
#else
#define K22_LOG(dwLevel, lpFile, dwLine, lpFunction, bWin32Error, ...)                                                 \
	K22LogWrite(dwLevel, NULL, 0, NULL, bWin32Error, __VA_ARGS__)
#endif

VOID K22LogWrite(
	DWORD dwLevel, LPCSTR lpFile, DWORD dwLine, LPCSTR lpFunction, DWORD dwWin32Error, LPCSTR lpFormat, ...
); // __attribute__((format(printf, 6, 7)));

#if K22_LEVEL_TRACE >= K22_LOGLEVEL
#define K22_T(...) K22_LOG(K22_LEVEL_TRACE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#else
#define K22_T(...)
#endif

#if K22_LEVEL_VERBOSE >= K22_LOGLEVEL
#define K22_V(...) K22_LOG(K22_LEVEL_VERBOSE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#else
#define K22_V(...)
#endif

#if K22_LEVEL_DEBUG >= K22_LOGLEVEL
#define K22_D(...) K22_LOG(K22_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, FALSE, __VA_ARGS__)
#else
#define K22_D(...)
#endif

#if K22_LEVEL_INFO >= K22_LOGLEVEL
#define K22_I(...) K22_LOG(K22_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, FALSE, __VA_ARGS__)
#else
#define K22_I(...)
#endif

#if K22_LEVEL_WARN >= K22_LOGLEVEL
#define K22_W(...)	   K22_LOG(K22_LEVEL_WARN, __FILE__, __LINE__, __FUNCTION__, 0, __VA_ARGS__)
#define K22_W_ERR(...) K22_LOG(K22_LEVEL_WARN, __FILE__, __LINE__, __FUNCTION__, GetLastError(), __VA_ARGS__)
#else
#define K22_W(...)
#define K22_W_ERR(...)
#endif

#if K22_LEVEL_ERROR >= K22_LOGLEVEL
#define K22_E(...)	   K22_LOG(K22_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 0, __VA_ARGS__)
#define K22_E_ERR(...) K22_LOG(K22_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, GetLastError(), __VA_ARGS__)
#define RETURN_K22_E(...)                                                                                              \
	do {                                                                                                               \
		K22_LOG(K22_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 0, __VA_ARGS__);                                    \
		return FALSE;                                                                                                  \
	} while (0)
#define RETURN_K22_E_ERR(...)                                                                                          \
	do {                                                                                                               \
		K22_LOG(K22_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, GetLastError(), __VA_ARGS__);                       \
		return FALSE;                                                                                                  \
	} while (0)
#else
#define K22_E(...)
#define K22_E_ERR(...)
#define RETURN_K22_E(...)                                                                                              \
	{ return FALSE; }
#define RETURN_K22_E_ERR(...)                                                                                          \
	{ return FALSE; }
#endif

#if K22_LEVEL_FATAL >= K22_LOGLEVEL
#define K22_F(...)	   K22_LOG(K22_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, 0, __VA_ARGS__)
#define K22_F_ERR(...) K22_LOG(K22_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, GetLastError(), __VA_ARGS__)
#define RETURN_K22_F(...)                                                                                              \
	do {                                                                                                               \
		K22_LOG(K22_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, 0, __VA_ARGS__);                                    \
		return FALSE;                                                                                                  \
	} while (0)
#define RETURN_K22_F_ERR(...)                                                                                          \
	do {                                                                                                               \
		K22_LOG(K22_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, GetLastError(), __VA_ARGS__);                       \
		return FALSE;                                                                                                  \
	} while (0)
#else
#define K22_F(...)
#define K22_F_ERR(...)
#define RETURN_K22_F(...)                                                                                              \
	{ return FALSE; }
#define RETURN_K22_F_ERR(...)                                                                                          \
	{ return FALSE; }
#endif
