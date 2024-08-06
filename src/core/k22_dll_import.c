// Copyright (c) Kuba Szczodrzyński 2024-2-25.

#include "kernel22.h"

BOOL K22LoadExtraDlls() {
	PK22_DLL_EXTRA pDllExtra = NULL;
	K22_LL_FOREACH(pK22Data->stDll.pDllExtra, pDllExtra) {
		if (pDllExtra->hModule != NULL)
			continue;
		K22_I("DLL Extra: loading %s (%s)", pDllExtra->lpKey, pDllExtra->lpTargetDll);
		pDllExtra->hModule = LoadLibrary(pDllExtra->lpTargetDll);
		if (pDllExtra->hModule == NULL)
			RETURN_K22_F_ERR("Couldn't load extra DLL - %s", pDllExtra->lpTargetDll);
	}
	return TRUE;
}

BOOL K22ProcessImports(LPVOID lpImageBase) {
	PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(lpImageBase);

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
		K22_I("Processing imports of %p (%s) - no imports found", lpImageBase, pK22ModuleData->lpModuleName);
		return TRUE;
	}

	K22_I("Processing imports of %p (%s)", lpImageBase, pK22ModuleData->lpModuleName);

	// process each import descriptor
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = RVA(dwImportDirectoryRva);
	for (/**/; pImportDesc->FirstThunk; pImportDesc++) {
		LPCSTR lpImportModuleName = RVA(pImportDesc->Name);
		K22_D("Module %s imports %s", pK22ModuleData->lpModuleName, lpImportModuleName);

		PULONG_PTR pThunk	  = RVA(pImportDesc->FirstThunk);
		PULONG_PTR pOrigThunk = RVA(pImportDesc->OriginalFirstThunk);
		if (pImportDesc->OriginalFirstThunk == 0)
			// apparently OFT can be NULL... in this case just use FTs instead
			// see: HWiNFO64.EXE
			pOrigThunk = pThunk;

		for (/**/; *pThunk != 0 && *pOrigThunk != 0; pThunk++, pOrigThunk++) {
			PVOID pProcAddress;
			LPCSTR lpSymbolName;

			CHAR szSymbolName[1 + 5 + 1] = "#";
			if (IMAGE_SNAP_BY_ORDINAL(*pOrigThunk)) {
				_itoa(IMAGE_ORDINAL(*pOrigThunk), szSymbolName + 1, 10);
				lpSymbolName = szSymbolName;
			} else {
				lpSymbolName = ((PIMAGE_IMPORT_BY_NAME)RVA(*pOrigThunk))->Name;
			}

			pProcAddress = K22ResolveSymbol(pK22ModuleData->lpModuleName, lpImportModuleName, lpSymbolName);
			if (pProcAddress == NULL)
				return FALSE;

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

VOID K22DisableInitRoutine(LPVOID lpImageBase) {
	PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(lpImageBase);
	PLDR_DATA_TABLE_ENTRY pLdrEntry = pK22ModuleData->pLdrEntry;

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

	// To fix it, let's disable the entry point of all DLLs that are loaded "in a controlled manner"
	// (e.g. during process initialization, or in hooked LoadLibrary() calls).
	// The entry point is replaced with a dummy method, instead of NULL, which will
	// (hopefully) allow ntdll to call TLS initializers.
	if (pK22Data->fDelayDllInit && pLdrEntry->EntryPoint && pLdrEntry->EntryPoint != K22DummyEntryPoint) {
		K22_V("Disabling init routine of %s at %p", pK22ModuleData->lpModuleName, pLdrEntry->EntryPoint);
		// keep the original one and replace it
		pK22ModuleData->lpDelayedInitRoutine = pLdrEntry->EntryPoint;
		pLdrEntry->EntryPoint				 = K22DummyEntryPoint;
	}
}

BOOL K22CallInitRoutines(LPVOID lpContext) {
	K22_LDR_ENUM(pLdrEntry, InInitializationOrderModuleList, InInitializationOrderLinks) {
		PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(pLdrEntry->DllBase);
		if (!pK22ModuleData->lpDelayedInitRoutine)
			continue;
		K22_D("Calling init routine of %s at %p", pK22ModuleData->lpModuleName, pK22ModuleData->lpDelayedInitRoutine);
		BOOL bRet = (pK22ModuleData->lpDelayedInitRoutine)(pLdrEntry->DllBase, DLL_PROCESS_ATTACH, lpContext);
		// restore the original entry point (for DLL unload, etc.)
		pLdrEntry->EntryPoint				 = pK22ModuleData->lpDelayedInitRoutine;
		pK22ModuleData->lpDelayedInitRoutine = NULL;
		if (bRet == FALSE)
			RETURN_K22_F("Init routine of %s failed", pK22ModuleData->lpModuleName);
	}
	return TRUE;
}

BOOL K22DummyEntryPoint(HANDLE hDll, DWORD dwReason, LPVOID lpContext) {
	return TRUE;
}
