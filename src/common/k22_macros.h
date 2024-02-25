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
		  ? (pNt)->stNt64.OptionalHeader.DataDirectory[eEntry]                                                         \
		  : (pNt)->stNt32.OptionalHeader.DataDirectory[eEntry])                                                        \
		 .VirtualAddress)

#define K22_LDR_ENUM(pLdrEntry)                                                                                        \
	for (PLDR_DATA_TABLE_ENTRY pLdrList	 = (PVOID)&NtCurrentPeb()->Ldr->InLoadOrderModuleList,                         \
							   pLdrEntry = (PVOID)((PLIST_ENTRY)pLdrList)->Flink;                                      \
		 pLdrEntry != pLdrList;                                                                                        \
		 pLdrEntry = (PVOID)pLdrEntry->InLoadOrderLinks.Flink)

// Memory allocation macros

#define K22_MALLOC(pVar)                                                                                               \
	do {                                                                                                               \
		pVar = HeapAlloc(GetProcessHeap(), 0, sizeof(*pVar));                                                          \
		if (pVar == NULL)                                                                                              \
			RETURN_K22_F_ERR("Couldn't allocate memory for " #pVar);                                                   \
	} while (0)

#define K22_MALLOC_LENGTH(pVar, cbLength)                                                                              \
	do {                                                                                                               \
		pVar = malloc(cbLength);                                                                                       \
		if (pVar == NULL)                                                                                              \
			RETURN_K22_F_ERR("Couldn't allocate memory for " #pVar);                                                   \
	} while (0)

#define K22_CALLOC(pVar)                                                                                               \
	do {                                                                                                               \
		pVar = malloc(sizeof(*pVar));                                                                                  \
		if (pVar == NULL)                                                                                              \
			RETURN_K22_F_ERR("Couldn't allocate memory for " #pVar);                                                   \
		memset(pVar, 0, sizeof(*pVar));                                                                                \
	} while (0)

#define K22_FREE(pVar) free(pVar)

// Linked list macros

#define K22_LL_PREPEND(pHead, pElem)	  DL_PREPEND2(pHead, pElem, pPrev, pNext)
#define K22_LL_APPEND(pHead, pElem)		  DL_APPEND2(pHead, pElem, pPrev, pNext)
#define K22_LL_DELETE(pHead, pElem)		  DL_DELETE2(pHead, pElem, pPrev, pNext)
#define K22_LL_FOREACH(pHead, pElem)	  DL_FOREACH2(pHead, pElem, pNext)
#define K22_LL_FOREACH_SAFE(pHead, pElem) DL_FOREACH_SAFE2(pHead, pElem, pPrev, pNext)

#define K22_LL_ALLOC_APPEND(pHead, pElem)                                                                              \
	do {                                                                                                               \
		K22_CALLOC(pElem);                                                                                             \
		K22_LL_APPEND(pHead, pElem);                                                                                   \
	} while (0)
#define K22_LL_ALLOC_PREPEND(pHead, pElem)                                                                             \
	do {                                                                                                               \
		K22_CALLOC(pElem);                                                                                             \
		K22_LL_PREPEND(pHead, pElem);                                                                                  \
	} while (0)
#define K22_LL_FIND(pHead, pElem, cond)                                                                                \
	do {                                                                                                               \
		K22_LL_FOREACH(pHead, pElem) {                                                                                 \
			if (cond)                                                                                                  \
				break;                                                                                                 \
		}                                                                                                              \
	} while (0)

// Registry macros

#define K22_REG_ACCESS KEY_READ

#define K22_REG_VARS()                                                                                                 \
	DWORD dwIndex, cbName, cbValue;                                                                                    \
	TCHAR szName[256 + 1], szValue[256 + 1]

#define K22_REG_REQUIRE_KEY(hParent, lpName, hChild)                                                                   \
	do {                                                                                                               \
		DWORD dwError;                                                                                                 \
		if ((dwError = RegOpenKeyEx(hParent, lpName, 0, K22_REG_ACCESS, &hChild)) != ERROR_SUCCESS)                    \
			RETURN_K22_F_ERR_VAL(dwError, "Couldn't open registry key '%s'", lpName);                                  \
	} while (0)

#define K22_REG_OPEN_KEY(hParent, lpName, hChild)                                                                      \
	(RegOpenKeyEx(hParent, lpName, 0, K22_REG_ACCESS, &hChild) == ERROR_SUCCESS)

#define K22_REG_REQUIRE_VALUE(hKey, lpName, pValue, cbValue)                                                           \
	do {                                                                                                               \
		DWORD dwError;                                                                                                 \
		if (cbValue = sizeof(pValue),                                                                                  \
			(dwError = RegGetValue(hKey, NULL, lpName, RRF_RT_ANY, NULL, (LPBYTE)pValue, &cbValue)) != ERROR_SUCCESS)  \
			RETURN_K22_F_ERR_VAL(dwError, "Couldn't read registry value '%s'", lpName);                                \
	} while (0)

#define K22_REG_READ_VALUE(hKey, lpName, pValue, cbValue)                                                              \
	(cbValue = sizeof(pValue),                                                                                         \
	 RegGetValue(hKey, NULL, lpName, RRF_RT_ANY, NULL, (LPBYTE)pValue, &cbValue) == ERROR_SUCCESS)

#define K22_REG_ENUM_KEY(hKey, pName, cbName)                                                                          \
	dwIndex = 0;                                                                                                       \
	while (1)                                                                                                          \
		if (cbName = sizeof(pName) - 1,                                                                                \
			RegEnumKeyEx(hKey, dwIndex++, pName, &cbName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)                    \
			break;                                                                                                     \
		else

#define K22_REG_ENUM_VALUE(hKey, pName, cbName, pValue, cbValue)                                                       \
	dwIndex = 0;                                                                                                       \
	while (1)                                                                                                          \
		if (cbName	= sizeof(pName) - 1,                                                                               \
			cbValue = sizeof(pValue),                                                                                  \
			RegEnumValue(hKey, dwIndex++, pName, &cbName, NULL, NULL, (LPBYTE)pValue, &cbValue) != ERROR_SUCCESS)      \
			break;                                                                                                     \
		else
