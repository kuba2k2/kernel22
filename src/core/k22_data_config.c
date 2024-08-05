// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

DWORD K22ConfigReadValueGlobal(LPCSTR lpName, PVOID pValue, DWORD cbValue) {
	if (RegGetValue(pK22Data->stReg.hMain, NULL, lpName, RRF_RT_ANY, NULL, pValue, &cbValue) != ERROR_SUCCESS)
		return 0;
	return cbValue;
}

DWORD K22ConfigReadValue(LPCSTR lpName, PVOID pValue, DWORD cbValue) {
	for (PHKEY pConfig = &pK22Data->stReg.hConfig[1]; pConfig >= &pK22Data->stReg.hConfig[0]; pConfig--) {
		HKEY hConfig = *pConfig;
		if (hConfig == NULL)
			continue;
		if (RegGetValue(hConfig, NULL, lpName, RRF_RT_ANY, NULL, pValue, &cbValue) == ERROR_SUCCESS)
			return cbValue;
	}
	return 0;
}

BOOL K22ConfigReadKey(LPCSTR lpName, BOOL (*pProc)(HKEY)) {
	for (PHKEY pConfig = &pK22Data->stReg.hConfig[0]; pConfig <= &pK22Data->stReg.hConfig[1]; pConfig++) {
		HKEY hConfig = *pConfig;
		if (hConfig == NULL)
			continue;
		HKEY hKey;
		if (K22_REG_OPEN_KEY(hConfig, lpName, hKey)) {
			if (!pProc(hKey)) {
				RegCloseKey(hKey);
				return FALSE;
			}
			RegCloseKey(hKey);
		}
	}
	return TRUE;
}

BOOL K22ConfigParseDllExtra(HKEY hDllExtra) {
	K22_REG_VARS();

	K22_REG_ENUM_VALUE(hDllExtra, szName, cbName, szValue, cbValue) {
		PK22_DLL_EXTRA pDllExtra;
		K22_LL_FIND(
			pK22Data->stDll.pDllExtra,
			pDllExtra,
			// comparison
			strcmp(szName, pDllExtra->lpKey) == 0
		);
		if (pDllExtra == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stDll.pDllExtra, pDllExtra);
			if (!K22StringDup(szName, cbName, &pDllExtra->lpKey))
				return FALSE;
		} else {
			K22_V(" - DLL Extra: will replace '%s'", pDllExtra->lpKey);
		}
		if (!K22StringDupFileName(szValue, cbValue - 1, &pDllExtra->lpTargetDll))
			return FALSE;
		K22_D(" - DLL Extra: setting '%s' (%s)", pDllExtra->lpKey, pDllExtra->lpTargetDll);
	}
	return TRUE;
}

static INT K22DllApiSetCompare(PVOID pDllApiSet1, PVOID pDllApiSet2) {
	return _stricmp(((PK22_DLL_API_SET)pDllApiSet1)->lpSourceDll, ((PK22_DLL_API_SET)pDllApiSet2)->lpSourceDll);
}

BOOL K22ConfigParseDllApiSet(HKEY hDllApiSet) {
	K22_REG_VARS();

	K22_REG_ENUM_VALUE(hDllApiSet, szName, cbName, szValue, cbValue) {
		PK22_DLL_API_SET pDllApiSet;
		K22_LL_ALLOC_APPEND(pK22Data->stDll.pDllApiSet, pDllApiSet);
		if (!K22StringDupDllTarget(szName, cbName, &pDllApiSet->lpSourceDll, &pDllApiSet->lpSourceSymbol))
			return FALSE;
		if (!K22StringDup(szValue, cbValue - 1, &pDllApiSet->lpTargetDll))
			return FALSE;

		K22_V(
			" - DLL ApiSet: setting %s!%s -> %s",
			pDllApiSet->lpSourceDll,
			pDllApiSet->lpSourceSymbol ? pDllApiSet->lpSourceSymbol : "*",
			pDllApiSet->lpTargetDll
		);
	}
	// sort the list; this is an optimization used together with "pDllApiSetDefault" in K22FindDllApiSet()
	K22_LL_SORT(pK22Data->stDll.pDllApiSet, K22DllApiSetCompare);
	return TRUE;
}

BOOL K22ConfigParseDllRedirect(HKEY hDllRedirect) {
	K22_REG_VARS();

	K22_REG_ENUM_VALUE(hDllRedirect, szName, cbName, szValue, cbValue) {
		PK22_DLL_REDIRECT pDllRedirect;
		K22_LL_FIND(
			pK22Data->stDll.pDllRedirect,
			pDllRedirect,
			// comparison
			strcmp(szName, pDllRedirect->lpSourceDll) == 0
		);
		if (pDllRedirect == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stDll.pDllRedirect, pDllRedirect);
			if (!K22StringDup(szName, cbName, &pDllRedirect->lpSourceDll))
				return FALSE;
		} else {
			K22_V(" - DLL Redirect: will replace %s", pDllRedirect->lpModuleName);
		}
		if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRedirect->lpTargetDll))
			return FALSE;
		K22_D(" - DLL Redirect: setting %s -> %s", pDllRedirect->lpSourceDll, pDllRedirect->lpTargetDll);
	}
	return TRUE;
}

BOOL K22ConfigParseDllRewrite(HKEY hDllRewrite) {
	K22_REG_VARS();

	K22_REG_ENUM_VALUE(hDllRewrite, szName, cbName, szValue, cbValue) {
		PK22_DLL_REWRITE pDllRewrite;
		K22_LL_FIND(
			pK22Data->stDll.pDllRewrite,
			pDllRewrite,
			// comparison
			strcmp(szName, pDllRewrite->lpSourceDll) == 0
		);
		if (pDllRewrite == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stDll.pDllRewrite, pDllRewrite);
			if (!K22StringDup(szName, cbName, &pDllRewrite->lpSourceDll))
				return FALSE;
		}
		if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRewrite->lpDefaultDll))
			return FALSE;
		K22_D(" - DLL Rewrite: setting %s!? (missing) -> %s", pDllRewrite->lpSourceDll, pDllRewrite->lpDefaultDll);
	}

	K22_REG_ENUM_KEY(hDllRewrite, szName, cbName) {
		PK22_DLL_REWRITE pDllRewrite;
		K22_LL_FIND(
			pK22Data->stDll.pDllRewrite,
			pDllRewrite,
			// comparison
			strcmp(szName, pDllRewrite->lpSourceDll) == 0
		);
		if (pDllRewrite == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stDll.pDllRewrite, pDllRewrite);
			if (!K22StringDup(szName, cbName, &pDllRewrite->lpSourceDll))
				return FALSE;
		}
		HKEY hDllRewriteItem;
		K22_REG_REQUIRE_KEY(hDllRewrite, szName, hDllRewriteItem);
		if (K22_REG_READ_VALUE(hDllRewriteItem, NULL, szValue, cbValue)) {
			if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRewrite->lpDefaultDll))
				return FALSE;
			K22_D(" - DLL Rewrite: setting %s!? (missing) -> %s", pDllRewrite->lpSourceDll, pDllRewrite->lpDefaultDll);
		}
		if (K22_REG_READ_VALUE(hDllRewriteItem, "*", szValue, cbValue)) {
			if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRewrite->lpCatchAllDll))
				return FALSE;
			K22_D(" - DLL Rewrite: setting %s!* (all) -> %s", pDllRewrite->lpSourceDll, pDllRewrite->lpCatchAllDll);
		}
		K22_REG_ENUM_VALUE(hDllRewriteItem, szName, cbName, szValue, cbValue) {
			if (szName[0] == '\0' || szName[0] == '*') // skip Default and Catch-All values
				continue;
			PK22_DLL_REWRITE_SYMBOL pSymbol;
			K22_LL_FIND(
				pDllRewrite->pSymbols,
				pSymbol,
				// comparison
				strcmp(szName, pSymbol->lpSourceSymbol) == 0
			);
			if (pSymbol == NULL) {
				K22_LL_ALLOC_APPEND(pDllRewrite->pSymbols, pSymbol);
				if (!K22StringDup(szName, cbName, &pSymbol->lpSourceSymbol))
					return FALSE;
			} else {
				K22_V(" - DLL Rewrite: will replace %s!%s", pDllRewrite->lpSourceDll, pSymbol->lpSourceSymbol);
			}
			if (!K22StringDupDllTarget(szValue, cbValue - 1, &pSymbol->lpTargetDll, &pSymbol->lpTargetSymbol))
				return FALSE;
			if (pSymbol->lpTargetSymbol == NULL) {
				pSymbol->lpTargetSymbol = pSymbol->lpSourceSymbol;
			}
			K22_D(
				" - DLL Rewrite: setting %s!%s -> %s!%s",
				pDllRewrite->lpSourceDll,
				pSymbol->lpSourceSymbol,
				pSymbol->lpTargetDll,
				pSymbol->lpTargetSymbol
			);
		}
	}
	return TRUE;
}
