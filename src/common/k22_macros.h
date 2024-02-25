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

// Memory allocation macros

#define K22_MALLOC(pVar)                                                                                               \
	do {                                                                                                               \
		pVar = HeapAlloc(GetProcessHeap(), 0, sizeof(*pVar));                                                          \
		if (pVar == NULL)                                                                                              \
			RETURN_K22_F_ERR("Couldn't allocate memory for " #pVar);                                                   \
	} while (0)

#define K22_MALLOC_LENGTH(pVar, cbLength)                                                                              \
	do {                                                                                                               \
		pVar = HeapAlloc(GetProcessHeap(), 0, cbLength);                                                               \
		if (pVar == NULL)                                                                                              \
			RETURN_K22_F_ERR("Couldn't allocate memory for " #pVar);                                                   \
	} while (0)

#define K22_CALLOC(pVar)                                                                                               \
	do {                                                                                                               \
		pVar = HeapAlloc(GetProcessHeap(), 0, sizeof(*pVar));                                                          \
		if (pVar == NULL)                                                                                              \
			RETURN_K22_F_ERR("Couldn't allocate memory for " #pVar);                                                   \
		ZeroMemory(pVar, sizeof(*pVar));                                                                               \
	} while (0)

// Linked list macros

#define K22_LL_APPEND(pType, pCurrent, ppNext)                                                                         \
	do {                                                                                                               \
		K22_CALLOC((*(pType *)ppNext));                                                                                \
		(*(pType *)ppNext)->pPrev = pCurrent;                                                                          \
		pCurrent				  = *(pType *)ppNext;                                                                  \
		ppNext					  = &pCurrent->pNext;                                                                  \
	} while (0)

#define K22_LL_END(pType, pStart, pCurrent, ppNext)                                                                    \
	do {                                                                                                               \
		ppNext	 = &pStart;                                                                                            \
		pCurrent = pStart;                                                                                             \
		while (*(pType *)ppNext) {                                                                                     \
			pCurrent = *(pType *)ppNext;                                                                               \
			ppNext	 = &pCurrent->pNext;                                                                               \
		}                                                                                                              \
                                                                                                                       \
	} while (0)

// Registry macros

#define K22_REG_ACCESS (KEY_READ | KEY_QUERY_VALUE)

#define K22_REG_REQUIRE_KEY(hParent, lpName, hChild)                                                                   \
	do {                                                                                                               \
		if (RegOpenKeyEx(hParent, lpName, 0, K22_REG_ACCESS, &hChild) != ERROR_SUCCESS)                                \
			RETURN_K22_F_ERR("Couldn't open registry key '%s'", lpName);                                               \
	} while (0)

#define K22_REG_OPEN_KEY(hParent, lpName, hChild)                                                                      \
	(RegOpenKeyEx(hParent, lpName, 0, K22_REG_ACCESS, &hChild) == ERROR_SUCCESS)

#define K22_REG_REQUIRE_VALUE(hKey, lpName, abValue, cbValue)                                                          \
	do {                                                                                                               \
		if ((dwError = RegGetValue(hKey, NULL, lpName, RRF_RT_ANY, NULL, (LPBYTE)abValue, &cbValue)) != ERROR_SUCCESS) \
			RETURN_K22_F_ERR_VAL(dwError, "Couldn't read registry value '%s'", lpName);                                \
	} while (0)

#define K22_REG_ENUM_VALUE(hKey, szName, cbName, abValue, cbValue)                                                     \
	dwIndex = 0;                                                                                                       \
	while (1)                                                                                                          \
		if (cbName	= sizeof(szName) - 1,                                                                              \
			cbValue = sizeof(abValue) - 1,                                                                             \
			RegEnumValue(hKey, dwIndex++, szName, &cbName, NULL, NULL, (LPBYTE)abValue, &cbValue) != ERROR_SUCCESS)    \
			break;                                                                                                     \
		else
