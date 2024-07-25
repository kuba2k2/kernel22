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
	DWORD dwOldProtect;

	K22_I("Processing imports of %p (%s)", lpImageBase, pK22ModuleData->lpModuleName);

	// get import directory pointer
#if K22_BITS64
	PDWORD pImportDirectoryRva =
		&pK22ModuleData->pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
#elif K22_BITS32
	PDWORD pImportDirectoryRva =
		&pK22ModuleData->pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
#endif
	DWORD dwImportDirectoryRva = *pImportDirectoryRva;
	if (dwImportDirectoryRva == 0) {
		K22_I("Image does not import any DLLs (no import directory)");
		return TRUE;
	}

	// process each import descriptor
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = RVA(dwImportDirectoryRva);
	for (/**/; pImportDesc->FirstThunk; pImportDesc++) {
		K22_I("Module %s imports %s", pK22ModuleData->lpModuleName, RVA(pImportDesc->Name));
		HANDLE hModule = GetModuleHandle(RVA(pImportDesc->Name));
		if (hModule == NULL) {
			K22_D(" - not found, importing");
			hModule = LoadLibrary(RVA(pImportDesc->Name));
		}
		if (hModule == NULL) {
			RETURN_K22_F_ERR("Couldn't import module %s", RVA(pImportDesc->Name));
		}

		PULONG_PTR pThunk	  = RVA(pImportDesc->FirstThunk);
		PULONG_PTR pOrigThunk = RVA(pImportDesc->OriginalFirstThunk);
		for (/**/; *pThunk != 0 && *pOrigThunk != 0; pThunk++, pOrigThunk++) {
			if (!K22UnlockMemory(*pThunk))
				RETURN_K22_F_ERR("Couldn't unlock thunk @ %p", pThunk);

			PVOID pProcAddress;
			if (IMAGE_SNAP_BY_ORDINAL(*pOrigThunk)) {
				pProcAddress = GetProcAddress(hModule, (PVOID)IMAGE_ORDINAL(*pOrigThunk));
				K22_D(
					"Module %s imports %s!#%lu -> %p",
					pK22ModuleData->lpModuleName,
					RVA(pImportDesc->Name),
					IMAGE_ORDINAL(*pOrigThunk),
					pProcAddress
				);
			} else {
				PIMAGE_IMPORT_BY_NAME pImportByName = RVA(*pOrigThunk);
				pProcAddress						= GetProcAddress(hModule, pImportByName->Name);
				K22_D(
					"Module %s imports %s!%s -> %p",
					pK22ModuleData->lpModuleName,
					RVA(pImportDesc->Name),
					pImportByName->Name,
					pProcAddress
				);
			}

			if (pProcAddress == 0) {
				RETURN_K22_F_ERR("Couldn't find symbol in module %s", RVA(pImportDesc->Name));
			}

			// snap even if (pThunk != pOrigThunk)
			// while it may look like snapped already, it's not always the case
			// see: VERSION.DLL which has kind-of-pre-snapped thunks
			*pThunk = (ULONG_PTR)pProcAddress;
		}

		// disable the import descriptor, so that ntdll.dll doesn't use it anymore
		// otherwise it would re-snap all thunks after the DLL notification callback
		if (!K22UnlockMemory(*pImportDesc))
			RETURN_K22_F_ERR("Couldn't unlock import descriptor @ %p", pImportDesc);
		pImportDesc->FirstThunk			= 0;
		pImportDesc->OriginalFirstThunk = 0;
	}

	return TRUE;
}
