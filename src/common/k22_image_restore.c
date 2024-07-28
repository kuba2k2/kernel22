// Copyright (c) Kuba Szczodrzyński 2024-2-22.

#include "kernel22.h"

BOOL K22RestoreImportTable(LPVOID lpImageBase) {
	// get import directory
	DWORD dwImportDirectoryRva = K22_NT_DATA_RVA(pK22Data->pNt, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (dwImportDirectoryRva == 0)
		RETURN_K22_F("Image does not import any DLLs! (no import directory)");
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = RVA(dwImportDirectoryRva);
	// get first thunk
	DWORD dwFirstThunkRva = pImportDescriptor[0].FirstThunk;
	if (dwFirstThunkRva == 0)
		RETURN_K22_F("Image does not import any DLLs! (no first thunk)");
	PULONG_PTR pFirstThunk = RVA(dwFirstThunkRva);

	// restore original import directory
	K22WithUnlockedLength(pImportDescriptor, sizeof(*pImportDescriptor) * 2) {
		pImportDescriptor[0] = pK22Data->pK22Header->stOrigImportDescriptor[0];
		pImportDescriptor[1] = pK22Data->pK22Header->stOrigImportDescriptor[1];
	}
	K22WithUnlockedLength(pFirstThunk, sizeof(*pFirstThunk) * 2) {
		pFirstThunk[0] = pK22Data->pK22Header->ullOrigFirstThunk[0];
		pFirstThunk[1] = pK22Data->pK22Header->ullOrigFirstThunk[1];
	}

	return TRUE;
}
