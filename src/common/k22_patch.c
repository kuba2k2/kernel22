// Copyright (c) Kuba Szczodrzyński 2024-2-22.

#include "kernel22.h"

extern BOOL K22PatchImportTableImpl(
	BYTE bSource,
	PIMAGE_K22_HEADER pK22Header,
	PIMAGE_NT_HEADERS3264 pNt,
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor,
	PULONGLONG pFirstThunk
);
extern BOOL K22RestoreImportTableImpl(
	PIMAGE_K22_HEADER pK22Header,
	PIMAGE_NT_HEADERS3264 pNt,
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor,
	PULONGLONG pFirstThunk
);

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

	// patch the import table
	K22WithUnlocked(*pK22Header) {
		K22WithUnlocked(*pNt) {
			K22WithUnlockedLength(pImportDescriptor, sizeof(*pImportDescriptor) * 2) {
				K22WithUnlockedLength(pFirstThunk, sizeof(*pFirstThunk) * 2) {
					if (bSource != K22_SOURCE_NONE) {
						if (!K22PatchImportTableImpl(bSource, pK22Header, pNt, pImportDescriptor, pFirstThunk))
							return FALSE;
					} else {
						if (!K22RestoreImportTableImpl(pK22Header, pNt, pImportDescriptor, pFirstThunk))
							return FALSE;
					}
				}
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

	// patch the import table
	if (bSource != K22_SOURCE_NONE) {
		if (!K22PatchImportTableImpl(bSource, &stK22Header, &stNt, stImportDescriptor, ullFirstThunk))
			return FALSE;
	} else {
		if (!K22RestoreImportTableImpl(&stK22Header, &stNt, stImportDescriptor, ullFirstThunk))
			return FALSE;
	}

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

	// patch the import table
	if (bSource != K22_SOURCE_NONE) {
		if (!K22PatchImportTableImpl(bSource, &stK22Header, &stNt, stImportDescriptor, ullFirstThunk))
			return FALSE;
	} else {
		if (!K22RestoreImportTableImpl(&stK22Header, &stNt, stImportDescriptor, ullFirstThunk))
			return FALSE;
	}

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

BOOL K22ClearBoundImportTable(LPVOID lpImageBase) {
	// get DOS header as K22 header
	PIMAGE_K22_HEADER pK22Header = (PIMAGE_K22_HEADER)lpImageBase;
	// get NT header
	PIMAGE_NT_HEADERS3264 pNt = RVA(pK22Header->dwPeRva);

	K22WithUnlocked(*pNt) {
		PIMAGE_DATA_DIRECTORY pDataDirectory;
		if (pNt->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
			pDataDirectory = &pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
		} else if (pNt->stNt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
			pDataDirectory = &pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
		} else {
			RETURN_K22_F("Unrecognized OptionalHeader magic %04X", pNt->stNt32.OptionalHeader.Magic);
		}
		if (pDataDirectory->VirtualAddress && pDataDirectory->Size) {
			pDataDirectory->VirtualAddress = 0;
			pDataDirectory->Size		   = 0;
		}
	}
	return TRUE;
}
