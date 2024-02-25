// Copyright (c) Kuba Szczodrzyński 2024-2-25.

#include "kernel22.h"

BOOL K22ImportTableRestore(LPVOID lpImageBase) {
	DWORD dwOldProtect;

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

	// unlock import directory
	if (!K22UnlockMemoryLength((PVOID)pImportDescriptor, sizeof(*pImportDescriptor) * 2))
		RETURN_K22_F_ERR("Couldn't unlock import directory");
	// unlock first thunk
	if (!K22UnlockMemoryLength((PVOID)pFirstThunk, sizeof(*pFirstThunk) * 2))
		RETURN_K22_F_ERR("Couldn't unlock first thunk");

	// restore original import directory
	pImportDescriptor[0] = pK22Data->pK22Header->stOrigImportDescriptor[0];
	pImportDescriptor[1] = pK22Data->pK22Header->stOrigImportDescriptor[1];
	pFirstThunk[0]		 = pK22Data->pK22Header->ullOrigFirstThunk[0];
	pFirstThunk[1]		 = pK22Data->pK22Header->ullOrigFirstThunk[1];

	return TRUE;
}

BOOL K22ProcessImports(LPVOID lpImageBase) {
	PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(lpImageBase);

	K22_I("Processing imports of %p (%s)", lpImageBase, pK22ModuleData->lpModuleName);

	// get import directory
	DWORD dwImportDirectoryRva = K22_NT_DATA_RVA(pK22ModuleData->pNt, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (dwImportDirectoryRva == 0)
		RETURN_K22_F("Image does not import any DLLs! (no import directory)");

	DWORD dwOldProtect;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = RVA(dwImportDirectoryRva);
	for (/**/; pImportDesc->FirstThunk; pImportDesc++) {
		K22_D(
			"Module: %s, FT=%x, OFT=%x",
			RVA(pImportDesc->Name),
			pImportDesc->FirstThunk,
			pImportDesc->OriginalFirstThunk
		);

		HANDLE hModule = GetModuleHandle(RVA(pImportDesc->Name));
		if (hModule == NULL) {
			K22_D("Module not found, importing");
			hModule = LoadLibrary(RVA(pImportDesc->Name));
		}
		if (hModule == NULL) {
			RETURN_K22_F_ERR("Couldn't import module %s", RVA(pImportDesc->Name));
		}

		PULONG_PTR pThunk = RVA(pImportDesc->FirstThunk);
		for (/**/; *pThunk != 0; pThunk++) {
			if (!K22UnlockMemory(*pThunk))
				RETURN_K22_F_ERR("Couldn't unlock thunk @ %p", pThunk);

			PVOID pProcAddress;
			if (IMAGE_SNAP_BY_ORDINAL(*pThunk)) {
				pProcAddress = GetProcAddress(hModule, (PVOID)IMAGE_ORDINAL(*pThunk));
				K22_D("Symbol: Ordinal=%lu, Addr=%p", IMAGE_ORDINAL(*pThunk), pProcAddress);
			} else {
				PIMAGE_IMPORT_BY_NAME pImportByName = RVA(*pThunk);
				pProcAddress						= GetProcAddress(hModule, pImportByName->Name);
				K22_D("Symbol: Name=%s, Hint=%d, Addr=%p", pImportByName->Name, pImportByName->Hint, pProcAddress);
			}

			if (pProcAddress == 0) {
				RETURN_K22_F_ERR("Couldn't find symbol in module %s", RVA(pImportDesc->Name));
			}

			*pThunk = (ULONG_PTR)pProcAddress;
		}
	}

	return TRUE;
}
