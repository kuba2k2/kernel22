// Copyright (c) Kuba Szczodrzyński 2024-2-25.

#include "kernel22.h"

BOOL K22ProcessImports(LPVOID lpImageBase) {
	PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(lpImageBase);

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

	// At this point (in DLL notification) the data table entry is populated with the entry point.
	// Normally, when starting a process, the initialization routines of all DLLs
	// would be called after all static dependencies are loaded.
	// Here we're using LoadLibrary() to resolve symbols, which calls the initialization routine
	// of a single DLL.
	// That wouldn't be a problem, but some DLLs have circular dependencies.
	// For example: gdi32.dll -> user32.dll -> gdi32.dll -> ...
	// In this scenario, we have to import user32.dll when resolving imports of user32.dll.
	// This makes LoadLibrary() call user32's initialization routine, which expects gdi32.dll
	// to be fully resolved, which isn't the case.

	// To fix it, let's clear the entry point of all DLLs loaded "in a controlled manner"
	// (e.g. during process initialization, or in hooked LoadLibrary() calls)
	if (pK22Data->fDelayDllInit && !pK22ModuleData->fIsProcess && pK22ModuleData->pLdrEntry->EntryPoint) {
		pK22ModuleData->lpDelayedInitRoutine  = pK22ModuleData->pLdrEntry->EntryPoint;
		pK22ModuleData->pLdrEntry->EntryPoint = NULL;
	}

	// process each import descriptor
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = RVA(dwImportDirectoryRva);
	for (/**/; pImportDesc->FirstThunk; pImportDesc++) {
		K22_D("Module %s imports %s", pK22ModuleData->lpModuleName, RVA(pImportDesc->Name));

		PULONG_PTR pThunk	  = RVA(pImportDesc->FirstThunk);
		PULONG_PTR pOrigThunk = RVA(pImportDesc->OriginalFirstThunk);
		for (/**/; *pThunk != 0 && *pOrigThunk != 0; pThunk++, pOrigThunk++) {
			PVOID pProcAddress;
			if (IMAGE_SNAP_BY_ORDINAL(*pOrigThunk)) {
				pProcAddress = K22ResolveSymbol(RVA(pImportDesc->Name), NULL, IMAGE_ORDINAL(*pOrigThunk));
				K22_V(
					"Module %s imports %s!#%lu -> %p",
					pK22ModuleData->lpModuleName,
					RVA(pImportDesc->Name),
					IMAGE_ORDINAL(*pOrigThunk),
					pProcAddress
				);
				if (pProcAddress == 0) {
					RETURN_K22_F_ERR(
						"Couldn't resolve symbol %s!#%lu",
						RVA(pImportDesc->Name),
						IMAGE_ORDINAL(*pOrigThunk)
					);
				}
			} else {
				LPCSTR lpSymbolName = ((PIMAGE_IMPORT_BY_NAME)RVA(*pOrigThunk))->Name;
				pProcAddress		= K22ResolveSymbol(RVA(pImportDesc->Name), lpSymbolName, 0);
				K22_V(
					"Module %s imports %s!%s -> %p",
					pK22ModuleData->lpModuleName,
					RVA(pImportDesc->Name),
					lpSymbolName,
					pProcAddress
				);
				if (pProcAddress == 0) {
					RETURN_K22_F_ERR("Couldn't resolve symbol %s!%s", RVA(pImportDesc->Name), lpSymbolName);
				}
			}

			K22WithUnlocked(*pThunk) {
				*pThunk = (ULONG_PTR)pProcAddress;
			}
		}

		// disable the import descriptor, so that ntdll.dll doesn't use it anymore
		// otherwise it would re-snap all thunks after the DLL notification callback
		K22WithUnlocked(*pImportDesc) {
			pImportDesc->FirstThunk			= 0;
			pImportDesc->OriginalFirstThunk = 0;
		}
	}

	return TRUE;
}

BOOL K22CallInitRoutines() {
	K22_LDR_ENUM(pLdrEntry, InInitializationOrderModuleList, InInitializationOrderLinks) {
		PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(pLdrEntry->DllBase);
		if (!pK22ModuleData->lpDelayedInitRoutine)
			continue;
		K22_I(
			"Calling initialization routine of %s at %p",
			pK22ModuleData->lpModuleName,
			pK22ModuleData->lpDelayedInitRoutine
		);
		(pK22ModuleData->lpDelayedInitRoutine)((pK22ModuleData->pLdrEntry->DllBase), (DLL_PROCESS_ATTACH), (NULL));
		// restore the original entry point (for DLL unload, etc.)
		pK22ModuleData->pLdrEntry->EntryPoint = pK22ModuleData->lpDelayedInitRoutine;
		pK22ModuleData->lpDelayedInitRoutine  = NULL;
	}
	return TRUE;
}
