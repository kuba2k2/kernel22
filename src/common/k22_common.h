// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#pragma once

#include "kernel22.h"

typedef struct {
	// e_res[0], e_res[1]
	BYTE abPatcherCookie[3];
	BYTE bPatcherType;
	// e_res[2], e_res[3]
	DWORD dwUnused1;

	// e_oemid, e_oeminfo
	DWORD _dwOem;

	// e_res2[0], e_res2[1]
	DWORD dwRvaImportDirectory;
	// e_res2[2], e_res2[3], ...
	DWORD dwUnused2[4];
} K22_HDR_DATA, *PK22_HDR_DATA;

#define K22_DOS_HDR_DATA(pDosHeader) ((PK22_HDR_DATA)(&((PIMAGE_DOS_HEADER)(pDosHeader))->e_res))

#define K22_PATCHER_COOKIE	"K22"
#define K22_PATCHER_PROCESS 'P'
#define K22_PATCHER_FILE	'F'

// Process memory macros

#define K22ReadProcessMemory(hProcess, lpBaseAddress, ullOffset, vOut)                                                 \
	ReadProcessMemory(hProcess, (LPCVOID)((ULONGLONG)lpBaseAddress + ullOffset), &vOut, sizeof(vOut), NULL)
#define K22ReadProcessMemoryLength(hProcess, lpBaseAddress, ullOffset, pvOut, cbLength)                                \
	ReadProcessMemory(hProcess, (LPCVOID)((ULONGLONG)lpBaseAddress + ullOffset), pvOut, cbLength, NULL)
#define K22ReadProcessMemoryArray(hProcess, lpBaseAddress, ullOffset, pvOut)                                           \
	ReadProcessMemory(hProcess, (LPCVOID)((ULONGLONG)lpBaseAddress + ullOffset), pvOut, sizeof(pvOut), NULL)

#define K22WriteProcessMemory(hProcess, lpBaseAddress, ullOffset, vIn)                                                 \
	WriteProcessMemory(hProcess, (LPVOID)((ULONGLONG)lpBaseAddress + ullOffset), &vIn, sizeof(vIn), NULL)
#define K22WriteProcessMemoryLength(hProcess, lpBaseAddress, ullOffset, pvIn, cbLength)                                \
	WriteProcessMemory(hProcess, (LPVOID)((ULONGLONG)lpBaseAddress + ullOffset), pvIn, cbLength, NULL)
#define K22WriteProcessMemoryArray(hProcess, lpBaseAddress, ullOffset, pvIn)                                           \
	WriteProcessMemory(hProcess, (LPVOID)((ULONGLONG)lpBaseAddress + ullOffset), pvIn, sizeof(pvIn), NULL)

#define K22UnlockProcessMemory(hProcess, lpBaseAddress, ullOffset, cbLength, lpOldProtect)                             \
	VirtualProtectEx(hProcess, (LPVOID)((ULONGLONG)lpBaseAddress + ullOffset), cbLength, PAGE_READWRITE, lpOldProtect)

// File macros

#define K22ReadFile(hFile, lOffset, vOut, pBytesRead)                                                                  \
	(SetFilePointer(hFile, lOffset, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&                                   \
	 ReadFile(hFile, &vOut, sizeof(vOut), pBytesRead, NULL))
#define K22ReadFileLength(hFile, lOffset, pvOut, cbLength, pBytesRead)                                                 \
	(SetFilePointer(hFile, lOffset, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&                                   \
	 ReadFile(hFile, pvOut, cbLength, pBytesRead, NULL))
#define K22ReadFileArray(hFile, lOffset, pvOut, pBytesRead)                                                            \
	(SetFilePointer(hFile, lOffset, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&                                   \
	 ReadFile(hFile, pvOut, sizeof(pvOut), pBytesRead, NULL))

#define K22WriteFile(hProcess, lOffset, vIn, pBytesWritten)                                                            \
	(SetFilePointer(hFile, lOffset, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&                                   \
	 WriteFile(hFile, &vIn, sizeof(vIn), pBytesWritten, NULL))
#define K22WriteFileLength(hProcess, lOffset, pvIn, cbLength, pBytesWritten)                                           \
	(SetFilePointer(hFile, lOffset, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&                                   \
	 WriteFile(hFile, pvIn, cbLength, pBytesWritten, NULL))
#define K22WriteFileArray(hProcess, lOffset, pvIn, pBytesWritten)                                                      \
	(SetFilePointer(hFile, lOffset, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&                                   \
	 WriteFile(hFile, pvIn, sizeof(pvIn), pBytesWritten, NULL))
