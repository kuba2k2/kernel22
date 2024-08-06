// Copyright (c) Kuba Szczodrzyński 2024-8-6.

#include "kernel22.h"

PK22_DLL_API_SET K22FindDllApiSet(LPCSTR lpModuleName, LPCSTR lpSymbolName) {
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
	if (pDllApiSetSameLevel)
		return pDllApiSetSameLevel;
	if (pDllApiSetSameName)
		return pDllApiSetSameName;
	K22_E("ApiSet not resolved! %s!%s", lpModuleName, lpSymbolName);
	return NULL;
}

PK22_DLL_REDIRECT K22FindDllRedirect(LPCSTR lpModuleName) {
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

PK22_DLL_REWRITE K22FindDllRewrite(LPCSTR lpModuleName) {
	PK22_DLL_REWRITE pDllRewrite = NULL;
	K22_LL_FIND(
		pK22Data->stDll.pDllRewrite,
		pDllRewrite,
		// comparison
		K22PathMatches(lpModuleName, pDllRewrite->lpSourceDll)
	);
	return pDllRewrite;
}

PK22_DLL_REWRITE_SYMBOL K22FindDllRewriteSymbol(PK22_DLL_REWRITE pDllRewrite, LPCSTR lpSymbolName) {
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
