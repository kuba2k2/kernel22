// Copyright (c) Kuba Szczodrzyński 2024-2-22.

#include "kernel22.h"

#undef RtlCopyMemory
#undef RtlZeroMemory

void RtlCopyMemory(void *Destination, const void *Source, size_t Length);
void RtlZeroMemory(void *Destination, size_t Length);

static BOOL K22PatchImportTableImpl(
	BYTE bSource, PIMAGE_K22_HEADER pK22Header, PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor, PULONGLONG pFirstThunk
) {
	// make sure there's enough room in the DOS stub
	if (pK22Header->dwPeRva < sizeof(*pK22Header))
		RETURN_K22_F(
			"PE header overlaps DOS stub! Not enough free space. (0x%X < 0x%X)",
			pK22Header->dwPeRva,
			sizeof(*pK22Header)
		);

	// always set the latest load source
	pK22Header->bSource = bSource;
	// exit if the image has been patched
	if (RtlCompareMemory(pK22Header->bCookie, K22_COOKIE, 3) == 3) {
		K22_W("Image has already been patched!");
		return TRUE;
	}
	// otherwise write the patch cookie
	RtlCopyMemory(pK22Header->bCookie, K22_COOKIE, 3);

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

BOOL K22PatchImportTable(BYTE bSource, LPVOID lpImageBase) {
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

	// modify the import table
	K22WithUnlocked(*pK22Header) {
		K22WithUnlockedLength(pImportDescriptor, sizeof(*pImportDescriptor) * 2) {
			K22WithUnlockedLength(pFirstThunk, sizeof(*pFirstThunk) * 2) {
				if (!K22PatchImportTableImpl(bSource, pK22Header, pImportDescriptor, pFirstThunk))
					return FALSE;
			}
		}
	}

	return TRUE;
}

#if !K22_VERIFIER

BOOL K22PatchImportTableProcess(BYTE bSource, HANDLE hProcess, LPVOID lpImageBase) {
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
	if (!K22PatchImportTableImpl(bSource, &stK22Header, stImportDescriptor, ullFirstThunk))
		return FALSE;

	// write modified DOS header
	K22WithUnlockedProcess(hProcess, RVA(0), sizeof(stK22Header)) {
		if (!K22WriteProcessMemory(hProcess, lpImageBase, 0, stK22Header))
			RETURN_K22_F_ERR("Couldn't write DOS header");
	}
	// write modified NT header
	K22WithUnlockedProcess(hProcess, RVA(stK22Header.dwPeRva), sizeof(stNt)) {
		if (!K22WriteProcessMemory(hProcess, lpImageBase, stK22Header.dwPeRva, stNt))
			RETURN_K22_F_ERR("Couldn't write NT header");
	}
	// write modified import directory
	K22WithUnlockedProcess(hProcess, RVA(dwImportDirectoryRva), sizeof(stImportDescriptor)) {
		if (!K22WriteProcessMemory(hProcess, lpImageBase, dwImportDirectoryRva, stImportDescriptor))
			RETURN_K22_F_ERR("Couldn't write import directory");
	}
	// write modified first thunk
	K22WithUnlockedProcess(hProcess, RVA(dwFirstThunkRva), sizeof(ullFirstThunk)) {
		if (!K22WriteProcessMemory(hProcess, lpImageBase, dwFirstThunkRva, ullFirstThunk))
			RETURN_K22_F_ERR("Couldn't write first thunk");
	}
	return TRUE;
}

static DWORD K22RvaToRaw(PIMAGE_SECTION_HEADER pSections, PIMAGE_SECTION_HEADER pSectionsEnd, DWORD dwRva) {
	for (PIMAGE_SECTION_HEADER pSection = pSections; pSection < pSectionsEnd; pSection++) {
		if (dwRva >= pSection->VirtualAddress && dwRva < pSection->VirtualAddress + pSection->Misc.VirtualSize) {
			return pSection->PointerToRawData + (dwRva - pSection->VirtualAddress);
		}
	}
	return 0;
}

BOOL K22PatchImportTableFile(BYTE bSource, HANDLE hFile) {
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
	if (!K22PatchImportTableImpl(bSource, &stK22Header, stImportDescriptor, ullFirstThunk))
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

static BOOL K22ClearBoundImportTableImpl(PIMAGE_NT_HEADERS3264 pNt) {
	if (pNt->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
		pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size			= 0;
	} else if (pNt->stNt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
		pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size			= 0;
	} else {
		RETURN_K22_F("Unrecognized OptionalHeader magic %04X", pNt->stNt32.OptionalHeader.Magic);
	}
	return TRUE;
}

BOOL K22ClearBoundImportTable(LPVOID lpImageBase) {
	// get DOS header as K22 header
	PIMAGE_K22_HEADER pK22Header = (PIMAGE_K22_HEADER)lpImageBase;
	// get NT header
	PIMAGE_NT_HEADERS3264 pNt = RVA(pK22Header->dwPeRva);

	// clear the bound import table
	K22WithUnlocked(*pNt) {
		if (!K22ClearBoundImportTableImpl(pNt))
			return FALSE;
	}

	return TRUE;
}

#if !K22_VERIFIER

BOOL K22ClearBoundImportTableProcess(HANDLE hProcess, LPVOID lpImageBase) {
	// read DOS header as K22 header
	IMAGE_K22_HEADER stK22Header;
	if (!K22ReadProcessMemory(hProcess, lpImageBase, 0, stK22Header))
		RETURN_K22_F_ERR("Couldn't read DOS header");
	// read NT header
	IMAGE_NT_HEADERS3264 stNt;
	if (!K22ReadProcessMemory(hProcess, lpImageBase, stK22Header.dwPeRva, stNt))
		RETURN_K22_F_ERR("Couldn't read NT header");

	// clear the bound import table
	if (!K22ClearBoundImportTableImpl(&stNt))
		return FALSE;

	// write modified NT header
	K22WithUnlockedProcess(hProcess, RVA(stK22Header.dwPeRva), sizeof(stNt)) {
		if (!K22WriteProcessMemory(hProcess, lpImageBase, stK22Header.dwPeRva, stNt))
			RETURN_K22_F_ERR("Couldn't write NT header");
	}
	return TRUE;
}

BOOL K22ClearBoundImportTableFile(HANDLE hFile) {
	DWORD dwFileBytes;

	// read DOS header as K22 header
	IMAGE_K22_HEADER stK22Header;
	if (!K22ReadFile(hFile, 0, stK22Header, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't read DOS header");
	// read NT header
	IMAGE_NT_HEADERS3264 stNt;
	if (!K22ReadFile(hFile, stK22Header.dwPeRva, stNt, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't read NT header");

	// clear the bound import table
	if (!K22ClearBoundImportTableImpl(&stNt))
		return FALSE;

	// write modified NT header
	if (!K22WriteFile(hFile, stK22Header.dwPeRva, stNt, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write NT header");
	return TRUE;
}

#endif
