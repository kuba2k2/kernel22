// Copyright (c) Kuba Szczodrzyński 2024-2-22.

#include "kernel22.h"

#undef RtlCopyMemory
#undef RtlZeroMemory

void RtlCopyMemory(void *Destination, const void *Source, size_t Length);
void RtlZeroMemory(void *Destination, size_t Length);

static BOOL K22ImportTablePatch(
	BYTE bSource, PIMAGE_K22_HEADER pK22Header, PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor, PULONGLONG pFirstThunk
) {
	// make sure there's enough room in the DOS stub
	if (pK22Header->dwPeRva < sizeof(*pK22Header))
		RETURN_K22_F(
			"PE header overlaps DOS stub! Not enough free space. (0x%X < 0x%X)",
			pK22Header->dwPeRva,
			sizeof(*pK22Header)
		);

	// set header cookie
	if (RtlCompareMemory(pK22Header->bCookie, K22_COOKIE, 3) == 3) {
		K22_W("Image has already been patched!");
		return TRUE;
	}
	RtlCopyMemory(pK22Header->bCookie, K22_COOKIE, 3);
	pK22Header->bSource = bSource;

	// copy original import directory
	pK22Header->stOrigImportDescriptor[0] = pImportDescriptor[0];
	pK22Header->stOrigImportDescriptor[1] = pImportDescriptor[1];
	pK22Header->ullOrigFirstThunk[0]	  = pFirstThunk[0];
	pK22Header->ullOrigFirstThunk[1]	  = pFirstThunk[1];

	// clear the import directory
	RtlZeroMemory((PVOID)pImportDescriptor, sizeof(*pImportDescriptor) * 2);
	RtlZeroMemory((PVOID)pFirstThunk, sizeof(*pFirstThunk) * 2);

	// rebuild the first import descriptor
	// point to name in DOS header
	pImportDescriptor->Name = (ULONG_PTR)&pK22Header->szModuleName - (ULONG_PTR)pK22Header;
	// point to original FT
	pImportDescriptor->FirstThunk = pK22Header->stOrigImportDescriptor[0].FirstThunk;
	// overwrite the original FT, point to name in DOS header
	pFirstThunk[0] = (ULONG_PTR)&pK22Header->wSymbolHint - (ULONG_PTR)pK22Header;

	// store names in the header
	RtlCopyMemory(pK22Header->szModuleName, K22_CORE_DLL, sizeof(K22_CORE_DLL));
	RtlCopyMemory(pK22Header->szSymbolName, K22_LOAD_SYMBOL, sizeof(K22_LOAD_SYMBOL));
	pK22Header->wSymbolHint = 0;

	return TRUE;
}

#if !K22_VERIFIER

BOOL K22ImportTablePatchProcess(BYTE bSource, HANDLE hProcess, LPVOID lpImageBase) {
	DWORD dwOldProtect;

	// read DOS header as K22 header
	IMAGE_K22_HEADER stK22Header;
	if (!K22ReadProcessMemory(hProcess, lpImageBase, 0, stK22Header))
		RETURN_K22_F_ERR("Couldn't read DOS header");
	// read NT header
	IMAGE_NT_HEADERS3264 stNt;
	if (!K22ReadProcessMemory(hProcess, lpImageBase, stK22Header.dwPeRva, stNt))
		RETURN_K22_F_ERR("Couldn't read NT header");
	// read import directory
	DWORD dwImportDirectoryRva = K22_NT_DATA_RVA(&stNt, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (dwImportDirectoryRva == 0)
		RETURN_K22_F("Image does not import any DLLs! (no import directory)");
	IMAGE_IMPORT_DESCRIPTOR stImportDescriptor[2];
	if (!K22ReadProcessMemory(hProcess, lpImageBase, dwImportDirectoryRva, stImportDescriptor))
		RETURN_K22_F_ERR("Couldn't read import directory");
	// read first thunk
	DWORD dwFirstThunkRva = stImportDescriptor[0].FirstThunk;
	if (dwFirstThunkRva == 0)
		RETURN_K22_F("Image does not import any DLLs! (no first thunk)");
	ULONG_PTR ullFirstThunk[2];
	if (!K22ReadProcessMemory(hProcess, lpImageBase, dwFirstThunkRva, ullFirstThunk))
		RETURN_K22_F_ERR("Couldn't read first thunk");

	// modify the import table
	if (!K22ImportTablePatch(bSource, &stK22Header, stImportDescriptor, ullFirstThunk))
		return FALSE;

	// write modified DOS header
	if (!K22UnlockProcessMemory(hProcess, lpImageBase, 0, sizeof(stK22Header), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock DOS header");
	if (!K22WriteProcessMemory(hProcess, lpImageBase, 0, stK22Header))
		RETURN_K22_F_ERR("Couldn't write DOS header");
	// write modified NT header
	if (!K22UnlockProcessMemory(hProcess, lpImageBase, stK22Header.dwPeRva, sizeof(stNt), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock NT header");
	if (!K22WriteProcessMemory(hProcess, lpImageBase, stK22Header.dwPeRva, stNt))
		RETURN_K22_F_ERR("Couldn't write NT header");
	// write modified import directory
	if (!K22UnlockProcessMemory(hProcess, lpImageBase, dwImportDirectoryRva, sizeof(stImportDescriptor), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock import directory");
	if (!K22WriteProcessMemory(hProcess, lpImageBase, dwImportDirectoryRva, stImportDescriptor))
		RETURN_K22_F_ERR("Couldn't write import directory");
	// write modified first thunk
	if (!K22UnlockProcessMemory(hProcess, lpImageBase, dwFirstThunkRva, sizeof(ullFirstThunk), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock first thunk");
	if (!K22WriteProcessMemory(hProcess, lpImageBase, dwFirstThunkRva, ullFirstThunk))
		RETURN_K22_F_ERR("Couldn't write first thunk");
	return TRUE;
}

#endif

BOOL K22ImportTablePatchImage(BYTE bSource, LPVOID lpImageBase) {
	DWORD dwOldProtect;

	// get DOS header as K22 header
	PIMAGE_K22_HEADER pK22Header = (PIMAGE_K22_HEADER)lpImageBase;
	// get NT header
	PIMAGE_NT_HEADERS3264 pNt = RVA(pK22Header->dwPeRva);
	// get import directory
	DWORD dwImportDirectoryRva = K22_NT_DATA_RVA(pNt, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (dwImportDirectoryRva == 0)
		RETURN_K22_F("Image does not import any DLLs! (no import directory)");
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = RVA(dwImportDirectoryRva);
	// get first thunk
	DWORD dwFirstThunkRva = pImportDescriptor[0].FirstThunk;
	if (dwFirstThunkRva == 0)
		RETURN_K22_F("Image does not import any DLLs! (no first thunk)");
	PULONG_PTR pFirstThunk = RVA(dwFirstThunkRva);

	// unlock DOS header
	if (!K22UnlockMemory(*pK22Header))
		RETURN_K22_F_ERR("Couldn't unlock DOS header");
	// unlock NT header
	if (!K22UnlockMemory(*pNt))
		RETURN_K22_F_ERR("Couldn't unlock NT header");
	// unlock import directory
	if (!K22UnlockMemoryLength((PVOID)pImportDescriptor, sizeof(*pImportDescriptor) * 2))
		RETURN_K22_F_ERR("Couldn't unlock import directory");
	// unlock first thunk
	if (!K22UnlockMemoryLength((PVOID)pFirstThunk, sizeof(*pFirstThunk) * 2))
		RETURN_K22_F_ERR("Couldn't unlock first thunk");

	// modify the import table
	if (!K22ImportTablePatch(bSource, pK22Header, pImportDescriptor, pFirstThunk))
		return FALSE;

	return TRUE;
}

#if !K22_VERIFIER

static DWORD K22RvaToRaw(PIMAGE_SECTION_HEADER pSections, PIMAGE_SECTION_HEADER pSectionsEnd, DWORD dwRva) {
	for (PIMAGE_SECTION_HEADER pSection = pSections; pSection < pSectionsEnd; pSection++) {
		if (dwRva >= pSection->VirtualAddress && dwRva < pSection->VirtualAddress + pSection->Misc.VirtualSize) {
			return pSection->PointerToRawData + (dwRva - pSection->VirtualAddress);
		}
	}
	return 0;
}

BOOL K22ImportTablePatchFile(BYTE bSource, HANDLE hFile) {
	DWORD dwFileBytes;

	// read DOS header as K22 header
	IMAGE_K22_HEADER stK22Header;
	if (!K22ReadFile(hFile, 0, stK22Header, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't read DOS header");
	// read NT header
	IMAGE_NT_HEADERS3264 stNt;
	if (!K22ReadFile(hFile, stK22Header.dwPeRva, stNt, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't read NT header");

	// read section headers
	SIZE_T cbSectionHeaders = sizeof(IMAGE_SECTION_HEADER) * stNt.stFile.NumberOfSections;
	DWORD dwSectionHeadersOffset =
		stK22Header.dwPeRva + sizeof(IMAGE_FILE_HEADER) + sizeof(DWORD) + stNt.stFile.SizeOfOptionalHeader;
	PIMAGE_SECTION_HEADER pSections = LocalAlloc(0, cbSectionHeaders);
	if (pSections == NULL)
		RETURN_K22_F_ERR("Couldn't allocate section headers");
	PIMAGE_SECTION_HEADER pSectionsEnd = pSections + stNt.stFile.NumberOfSections;
	if (!K22ReadFileLength(hFile, dwSectionHeadersOffset, pSections, cbSectionHeaders, &dwFileBytes)) {
		K22_F_ERR("Couldn't read section headers");
		goto error;
	}

	// read import directory
	DWORD dwImportDirectoryRva = K22_NT_DATA_RVA(&stNt, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (dwImportDirectoryRva == 0) {
		K22_F("Image does not import any DLLs! (no import directory)");
		goto error;
	}
	DWORD dwImportDirectoryRaw = K22RvaToRaw(pSections, pSectionsEnd, dwImportDirectoryRva);
	IMAGE_IMPORT_DESCRIPTOR stImportDescriptor[2];
	if (dwImportDirectoryRaw == 0) {
		K22_F("Couldn't find import directory");
		goto error;
	}
	if (!K22ReadFile(hFile, dwImportDirectoryRaw, stImportDescriptor, &dwFileBytes)) {
		K22_F_ERR("Couldn't read import directory");
		goto error;
	}

	// read first thunk
	DWORD dwFirstThunkRva = stImportDescriptor[0].FirstThunk;
	if (dwFirstThunkRva == 0) {
		K22_F("Image does not import any DLLs! (no first thunk)");
		goto error;
	}
	DWORD dwFirstThunkRaw = K22RvaToRaw(pSections, pSectionsEnd, dwFirstThunkRva);
	ULONG_PTR ullFirstThunk[2];
	if (dwFirstThunkRaw == 0) {
		K22_F("Couldn't find first thunk");
		goto error;
	}
	if (!K22ReadFile(hFile, dwFirstThunkRaw, ullFirstThunk, &dwFileBytes)) {
		K22_F_ERR("Couldn't read first thunk");
		goto error;
	}

	goto next;
error:
	LocalFree(pSections);
	return FALSE;
next:
	LocalFree(pSections);

	// modify the import table
	if (!K22ImportTablePatch(bSource, &stK22Header, stImportDescriptor, ullFirstThunk))
		return FALSE;

	// write modified DOS header
	if (!K22WriteFile(hFile, 0, stK22Header, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write DOS header");
	// write modified NT header
	if (!K22WriteFile(hFile, stK22Header.dwPeRva, stNt, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write NT header");
	// write modified import directory
	if (!K22WriteFile(hFile, dwImportDirectoryRaw, stImportDescriptor, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write import directory");
	// write modified first thunk
	if (!K22WriteFile(hFile, dwFirstThunkRaw, ullFirstThunk, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write first thunk");
	return TRUE;
}

#endif
