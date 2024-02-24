// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#pragma once

#include "kernel22.h"

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

// Memory macros

#define K22UnlockMemory(vIn)				  VirtualProtect(&vIn, sizeof(vIn), PAGE_READWRITE, &dwOldProtect)
#define K22UnlockMemoryLength(pvIn, cbLength) VirtualProtect(pvIn, cbLength, PAGE_READWRITE, &dwOldProtect)
#define K22UnlockMemoryArray(pvIn)			  VirtualProtect(pvIn, sizeof(pvIn), PAGE_READWRITE, &dwOldProtect)

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

// PE image helper macros

#define RVA(dwRva) ((LPVOID)((ULONG_PTR)(lpImageBase) + (dwRva)))

#define K22_NT_DATA_RVA(pNt, eEntry)                                                                                   \
	(((pNt)->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC                                              \
		  ? (pNt)->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]                                   \
		  : (pNt)->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT])                                  \
		 .VirtualAddress)
