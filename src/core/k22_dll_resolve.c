// Copyright (c) Kuba Szczodrzyński 2024-7-25.

#include "kernel22.h"

static PVOID K22LoadAndResolve(
	LPCSTR lpCallerName,
	LPCSTR lpModuleName,
	HINSTANCE *ppModule,
	LPCSTR lpSymbolName,
	PVOID *ppProc,
	LPCSTR lpModuleNameOrig,
	LPCSTR lpSymbolNameOrig,
	LPCSTR *ppErrorName
);

/*
 * Standard search order for unpackaged apps
 * https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order
 *
 * If safe DLL search mode is enabled, then the search order is as follows:
 *
 * 1. DLL Redirection.
 * 2. API sets.
 * 3. SxS manifest redirection.
 * 4. Loaded-module list.
 * 5. Known DLLs.
 * 6. N/A
 * 7. The folder from which the application loaded.
 * 8. The system folder. Use the GetSystemDirectory function to retrieve the path of this folder.
 * 9. N/A
 * 10. The Windows folder. Use the GetWindowsDirectory function to get the path of this folder.
 * 11. The current folder.
 * 12. The directories that are listed in the PATH environment variable.
 *
 * If safe DLL search mode is disabled, then the search order is the same except that the current folder
 * moves from position 11 to position 8 in the sequence (immediately after step 7).
 */
//

LPCSTR K22ResolveModulePath(LPCSTR lpModuleName, HINSTANCE *ppModule) {
	// resolve DLL name to full absolute path
	LPSTR lpModulePath;
	K22_MALLOC_LENGTH(lpModulePath, MAX_PATH);
	if (ppModule)
		*ppModule = NULL;

	// 0. Full path.
	if (K22PathIsFile(lpModuleName)) {
		strcpy(lpModulePath, lpModuleName);
		return lpModulePath;
	}
	// 4. Loaded-module list.
	K22_LDR_ENUM(pLdrEntry, InLoadOrderModuleList, InLoadOrderLinks) {
		wcstombs(lpModulePath, pLdrEntry->FullDllName.Buffer, MAX_PATH);
		if (K22PathMatches(lpModulePath, lpModuleName)) {
			if (ppModule)
				*ppModule = pLdrEntry->DllBase;
			return lpModulePath;
		}
	}
	// 7. The folder from which the application loaded.
	strcpy(lpModulePath, pK22Data->lpProcessDir);
	if (K22PathIsFileEx(lpModulePath, strlen(lpModulePath), lpModuleName))
		return lpModulePath;
	// 8. The system folder. Use the GetSystemDirectory function to retrieve the path of this folder.
	DWORD cchSystemDirectory = GetSystemDirectory(lpModulePath, MAX_PATH);
	if (K22PathIsFileEx(lpModulePath, cchSystemDirectory, lpModuleName))
		return lpModulePath;
	// 10. The Windows folder. Use the GetWindowsDirectory function to get the path of this folder.
	DWORD cchWindowsDirectory = GetWindowsDirectory(lpModulePath, MAX_PATH);
	if (K22PathIsFileEx(lpModulePath, cchWindowsDirectory, lpModuleName))
		return lpModulePath;
	// 11. The current folder.
	DWORD cchCurrentDirectory = GetCurrentDirectory(MAX_PATH, lpModulePath);
	if (K22PathIsFileEx(lpModulePath, cchCurrentDirectory, lpModuleName))
		return lpModulePath;
	// 12. The directories that are listed in the PATH environment variable.
	if (SearchPath(NULL, lpModuleName, NULL, MAX_PATH, lpModulePath, NULL) != 0)
		return lpModulePath;
	return NULL;
}

HINSTANCE K22ResolveModule(LPCSTR lpCallerName, LPCSTR lpModuleName) {
	LPCSTR lpModuleNameOrig = lpModuleName;
	LPCSTR lpErrorName		= NULL;

	HINSTANCE hModule	= NULL;
	HINSTANCE *ppModule = &hModule;

	PK22_DLL_API_SET pDllApiSet = K22FindDllApiSet(lpModuleName, NULL);
	if (pDllApiSet != NULL) {
		// apply DLL ApiSet redirect entry first
		lpModuleName = pDllApiSet->lpTargetDll;
		ppModule	 = &pDllApiSet->hModule; // cache module handle in redirect entry
	}

	PK22_DLL_REDIRECT pDllRedirect = K22FindDllRedirect(lpModuleName);
	if (pDllRedirect != NULL) {
		// then apply other DLL redirect entries
		lpModuleName = pDllRedirect->lpTargetDll;
		ppModule	 = &pDllRedirect->hModule; // cache module handle in redirect entry
	}

	if (K22LoadAndResolve(lpCallerName, lpModuleName, ppModule, NULL, NULL, lpModuleNameOrig, NULL, &lpErrorName))
		return *ppModule;

	K22_F_ERR("%s - %s -> %s -> %s", lpErrorName, lpCallerName, lpModuleNameOrig, lpModuleName);
	return NULL;
}

PVOID K22ResolveSymbol(LPCSTR lpCallerName, LPCSTR lpModuleName, LPCSTR lpSymbolName) {
	LPCSTR lpModuleNameOrig = lpModuleName;
	LPCSTR lpSymbolNameOrig = lpSymbolName;
	LPCSTR lpErrorName		= NULL;

	HINSTANCE hModule	= NULL;
	PVOID pProc			= NULL;
	HINSTANCE *ppModule = &hModule;
	PVOID *ppProc		= &pProc;

	PK22_DLL_API_SET pDllApiSet = K22FindDllApiSet(lpModuleName, lpSymbolName);
	if (pDllApiSet != NULL) {
		// apply DLL ApiSet redirect entry first
		lpModuleName = pDllApiSet->lpTargetDll;
		ppModule	 = &pDllApiSet->hModule; // cache module handle in redirect entry
	}

	PK22_DLL_REDIRECT pDllRedirect = K22FindDllRedirect(lpModuleName);
	if (pDllRedirect != NULL) {
		// then apply other DLL redirect entries
		lpModuleName = pDllRedirect->lpTargetDll;
		ppModule	 = &pDllRedirect->hModule; // cache module handle in redirect entry
	}

	PK22_DLL_REWRITE pDllRewrite = K22FindDllRewrite(lpModuleName);
	if (pDllRewrite == NULL) {
		// no DLL rewrite entry - nothing else to do
		if (K22LoadAndResolve(
				lpCallerName,
				lpModuleName,
				ppModule,
				lpSymbolName,
				ppProc,
				lpModuleNameOrig,
				lpSymbolNameOrig,
				&lpErrorName
			))
			return *ppProc;
		goto Error;
	}

	PK22_DLL_REWRITE_SYMBOL pDllRewriteSymbol = K22FindDllRewriteSymbol(pDllRewrite, lpSymbolName);
	if (pDllRewriteSymbol != NULL) {
		// got DLL rewrite symbol entry
		// return if already cached
		if (pDllRewriteSymbol->pProc)
			return pDllRewriteSymbol->pProc;
		// otherwise load and resolve
		lpModuleName = pDllRewriteSymbol->lpTargetDll;
		lpSymbolName = pDllRewriteSymbol->lpTargetSymbol;
		ppModule	 = &hModule;				  // nowhere to cache the module handle in
		ppProc		 = &pDllRewriteSymbol->pProc; // only cache the procedure address
		// this must resolve
		if (K22LoadAndResolve(
				lpCallerName,
				lpModuleName,
				ppModule,
				lpSymbolName,
				ppProc,
				lpModuleNameOrig,
				lpSymbolNameOrig,
				&lpErrorName
			))
			return *ppProc;
		goto Error;
	}

	// procedure not found so far - use Catch-All if set
	if (pDllRewrite->lpCatchAllDll != NULL) {
		// only return if valid procedure was found; also cache the module handle
		if (K22LoadAndResolve(
				lpCallerName,
				pDllRewrite->lpCatchAllDll,
				&pDllRewrite->hCatchAll,
				lpSymbolName,
				ppProc,
				lpModuleNameOrig,
				lpSymbolNameOrig,
				&lpErrorName
			))
			return *ppProc;
	}

	// try importing normally; will only cache in DLL redirect entry
	if (K22LoadAndResolve(
			lpCallerName,
			lpModuleName,
			ppModule,
			lpSymbolName,
			ppProc,
			lpModuleNameOrig,
			lpSymbolNameOrig,
			&lpErrorName
		))
		return *ppProc;

	// procedure still not found - use Default if set
	if (pDllRewrite->lpDefaultDll != NULL) {
		// only return if valid procedure was found; also cache the module handle
		if (K22LoadAndResolve(
				lpCallerName,
				pDllRewrite->lpDefaultDll,
				&pDllRewrite->hDefault,
				lpSymbolName,
				ppProc,
				lpModuleNameOrig,
				lpSymbolNameOrig,
				&lpErrorName
			))
			return *ppProc;
	}

	// nothing works
Error:
	K22_F_ERR(
		"%s - %s -> %s!%s -> %s!%s",
		lpErrorName,
		lpCallerName,
		lpModuleNameOrig,
		lpSymbolNameOrig,
		lpModuleName,
		lpSymbolName
	);
	return NULL;
}

static PVOID K22LoadAndResolve(
	LPCSTR lpCallerName,
	LPCSTR lpModuleName,
	HINSTANCE *ppModule,
	LPCSTR lpSymbolName,
	PVOID *ppProc,
	LPCSTR lpModuleNameOrig,
	LPCSTR lpSymbolNameOrig,
	LPCSTR *ppErrorName
) {
	if (ppProc != NULL && *ppProc != NULL)
		return *ppProc;

	LPCSTR lpModulePath = lpModuleName;
	if (*ppModule == NULL) {
		// resolve full module path, check if loaded already
		lpModulePath = K22ResolveModulePath(lpModuleName, ppModule);
		if (lpModulePath == NULL) {
			*ppErrorName = "Module not found";
			return NULL;
		}
	}
	if (*ppModule == NULL) {
		// otherwise load it by full path
		ANSI_STRING stModuleNameAnsi = {
			.Length		   = strlen(lpModuleName),
			.MaximumLength = 0,
			.Buffer		   = (LPSTR)lpModuleName,
		};
		UNICODE_STRING stModuleName;
		if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&stModuleName, &stModuleNameAnsi, TRUE))) {
			*ppErrorName = "String alloc failed";
			return NULL;
		}
		NTSTATUS ntStatus = K22RealLdrLoadDll(NULL, 0, &stModuleName, (PVOID *)ppModule);
		RtlFreeUnicodeString(&stModuleName);
		if (!NT_SUCCESS(ntStatus)) {
			*ppErrorName = "Module load failed";
			return NULL;
		}
		if (*ppModule == NULL) {
			*ppErrorName = "Module is NULL";
			return NULL;
		}
		PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(*ppModule);
		if (pK22ModuleData->fDllNotificationFailed) {
			*ppErrorName = "Module load failed in DLL notification";
			return NULL;
		}
	}

	// module handle is already loaded
	// return if no symbol to find
	if (lpSymbolName == NULL)
		return *ppModule;

	// otherwise find the procedure by ordinal or name
	if (lpSymbolName[0] == '#') {
		ULONG_PTR ulOrdinal = strtol(lpSymbolName + 1, NULL, 10);
		if (!NT_SUCCESS(K22RealLdrGetProcedureAddress(*ppModule, NULL, ulOrdinal, ppProc))) {
			*ppErrorName = "Ordinal not found";
			return NULL;
		}
	} else {
		ANSI_STRING stSymbolName = {
			.Length		   = strlen(lpSymbolName),
			.MaximumLength = 0,
			.Buffer		   = (LPSTR)lpSymbolName,
		};
		if (!NT_SUCCESS(K22RealLdrGetProcedureAddress(*ppModule, &stSymbolName, 0, ppProc))) {
			*ppErrorName = "Symbol not found";
			return NULL;
		}
	}

	PVOID pProc = *ppProc;
	if (pProc == NULL) {
		*ppErrorName = "Procedure not found";
		return NULL;
	}

	if (pK22Data->stConfig.bDebugImportResolver)
		K22_I(
			"Resolved - %s -> %s!%s -> %s!%s -> %p",
			lpCallerName,
			lpModuleNameOrig,
			lpSymbolNameOrig,
			lpModuleName,
			lpSymbolName,
			pProc
		);

	*ppErrorName = NULL;
	return pProc;
}
