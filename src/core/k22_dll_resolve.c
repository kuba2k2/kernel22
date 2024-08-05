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

static PK22_DLL_API_SET K22FindDllApiSet(LPCSTR lpModuleName, LPCSTR lpSymbolName) {
	// only try to resolve "*-ms-*" DLLs
	DWORD cchModuleName = strlen(lpModuleName);
	if (cchModuleName <= sizeof("api-ms-") || _strnicmp(lpModuleName + 3, "-ms-", 4) != 0)
		return NULL;

	PK22_DLL_API_SET pDllApiSet			 = NULL;
	PK22_DLL_API_SET pDllApiSetSameName	 = NULL;
	PK22_DLL_API_SET pDllApiSetSameLevel = NULL;
	K22_LL_FOREACH(pK22Data->stDll.pDllApiSet, pDllApiSet) {
		// compare module name (api-ms-aaa-bbb-lX-Y-Z.dll) without X, Y, Z
		if (_strnicmp(lpModuleName, pDllApiSet->lpSourceDll, cchModuleName - 9) == 0) {
			// module name matches
			if (pDllApiSet->lpSourceSymbol) {
				// quickly return any entry matching the source symbol
				if (_stricmp(pDllApiSet->lpSourceSymbol, lpSymbolName) == 0)
					return pDllApiSet;
				// otherwise skip this entry, because it's symbol-specific
				continue;
			}
			pDllApiSetSameName = pDllApiSet;
			// try comparing with X to find level matches
			if (_strnicmp(lpModuleName, pDllApiSet->lpSourceDll, cchModuleName - 8) == 0) {
				pDllApiSetSameLevel = pDllApiSet;
			}
			continue;
		}
		// pDllApiSet doesn't match the API set name
		// - check if match without source symbol already found
		// - return it already, since no more matches will be found
		//   (assuming that registry entries are sorted alphabetically)
		// - prioritize matched level than just the name
		if (pDllApiSetSameLevel != NULL)
			return pDllApiSetSameLevel;
		if (pDllApiSetSameName != NULL)
			return pDllApiSetSameName;
	}
	return pDllApiSetSameLevel ? pDllApiSetSameLevel : pDllApiSetSameName;
}

static PK22_DLL_REDIRECT K22FindDllRedirect(LPCSTR lpModuleName) {
	// recursively resolve configured DLL redirects
	// does NOT handle redirection loops!
	PK22_DLL_REDIRECT pDllRedirect = NULL;
	while (1) {
		PK22_DLL_REDIRECT pFoundItem;
		K22_LL_FIND(
			pK22Data->stDll.pDllRedirect,
			pFoundItem,
			// comparison
			K22PathMatches(lpModuleName, pFoundItem->lpSourceDll)
		);
		if (pFoundItem != NULL && pFoundItem != pDllRedirect) {
			pDllRedirect = pFoundItem;
			lpModuleName = pFoundItem->lpTargetDll;
			continue;
		}
		return pDllRedirect;
	}
}

static PK22_DLL_REWRITE K22FindDllRewrite(LPCSTR lpModuleName) {
	PK22_DLL_REWRITE pDllRewrite = NULL;
	K22_LL_FIND(
		pK22Data->stDll.pDllRewrite,
		pDllRewrite,
		// comparison
		K22PathMatches(lpModuleName, pDllRewrite->lpSourceDll)
	);
	return pDllRewrite;
}

static PK22_DLL_REWRITE_SYMBOL K22FindDllRewriteSymbol(PK22_DLL_REWRITE pDllRewrite, LPCSTR lpSymbolName) {
	if (pDllRewrite->pSymbols == NULL)
		return NULL;
	PK22_DLL_REWRITE_SYMBOL pDllRewriteSymbol = NULL;
	K22_LL_FIND(
		pDllRewrite->pSymbols,
		pDllRewriteSymbol,
		// comparison
		_stricmp(lpSymbolName, pDllRewriteSymbol->lpSourceSymbol) == 0
	);
	return pDllRewriteSymbol;
}

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

PVOID K22Resolve(LPCSTR lpCallerName, LPCSTR lpModuleName, LPCSTR lpSymbolName) {
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
	if (*ppProc != NULL)
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
		*ppModule = LoadLibrary(lpModulePath);
		if (*ppModule == NULL) {
			*ppErrorName = "Module load failed";
			return NULL;
		}
		PK22_MODULE_DATA pK22ModuleData = K22DataGetModule(*ppModule);
		if (pK22ModuleData->fDllNotificationFailed) {
			*ppErrorName = "Module load failed in DLL notification";
			return NULL;
		}
	}

	// module handle is already loaded
	// find the procedure by ordinal or name
	if (lpSymbolName[0] == '#') {
		ULONG_PTR ulOrdinal = strtol(lpSymbolName + 1, NULL, 10);
		*ppProc				= GetProcAddress(*ppModule, (PVOID)ulOrdinal);
	} else {
		*ppProc = GetProcAddress(*ppModule, lpSymbolName);
	}

	PVOID pProc = *ppProc;
	if (pProc == NULL) {
		*ppErrorName = "Symbol not found";
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
