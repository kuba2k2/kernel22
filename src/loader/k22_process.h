// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#pragma once

#include "kernel22.h"

BOOL K22PatchProcessVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor);
BOOL K22CreateDebugProcess(LPCSTR lpApplicationPath, LPCSTR lpCommandLine, LPPROCESS_INFORMATION lpProcessInformation);

#define K22ReadProcessMemory(hProcess, lpBaseAddress, ullOffset, vOut)                                                 \
	ReadProcessMemory(hProcess, (LPCVOID)((ULONGLONG)lpBaseAddress + ullOffset), &vOut, sizeof(vOut), NULL)
#define K22ReadProcessMemoryLength(hProcess, lpBaseAddress, ullOffset, pvOut, cbLength)                                \
	ReadProcessMemory(hProcess, (LPCVOID)((ULONGLONG)lpBaseAddress + ullOffset), pvOut, cbLength, NULL)
#define K22ReadProcessMemoryArray(hProcess, lpBaseAddress, ullOffset, pvOut)                                           \
	ReadProcessMemory(hProcess, (LPCVOID)((ULONGLONG)lpBaseAddress + ullOffset), pvOut, sizeof(pvOut), NULL)
