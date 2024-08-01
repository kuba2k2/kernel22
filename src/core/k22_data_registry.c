// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

static BOOL K22DataReadDllExtra(HKEY hDllExtra);
static BOOL K22DataReadDllRedirect(HKEY hDllRedirect);
static BOOL K22DataReadDllRewrite(HKEY hDllRewrite);
static BOOL K22DataReadWinVer(HKEY hWinVer);

BOOL K22DataReadRegistry() {
	K22_REG_VARS();

	K22_REG_REQUIRE_VALUE(pK22Data->stReg.hMain, "InstallDir", szValue, cbValue);
	cbValue--; // skip the NULL terminator
	if (szValue[cbValue - 1] != '\\') {
		szValue[cbValue++] = '\\';
	}
	if (pK22Data->fIs64Bit)
		strcpy(szValue + cbValue, "DLL_64\\");
	else
		strcpy(szValue + cbValue, "DLL_32\\");
	cbValue += 7;
	pK22Data->stConfig.lpInstallDir	 = _strdup(szValue);
	pK22Data->stConfig.cchInstallDir = cbValue;

	K22_REG_READ_VALUE(
		pK22Data->stReg.hMain,
		"DllNotificationMode",
		&pK22Data->stConfig.dwDllNotificationMode,
		cbValue
	);

	if (pK22Data->stDll.pWinVer == NULL) {
		K22_CALLOC(pK22Data->stDll.pWinVer);
		pK22Data->stDll.pWinVer->stDefault.dwMajor = NtCurrentPeb()->OSMajorVersion;
		pK22Data->stDll.pWinVer->stDefault.dwMinor = NtCurrentPeb()->OSMinorVersion;
		pK22Data->stDll.pWinVer->stDefault.dwBuild = NtCurrentPeb()->OSBuildNumber;
	}

	for (PHKEY pConfig = pK22Data->stReg.hConfig; pConfig < pK22Data->stReg.hConfig + 2; pConfig++) {
		HKEY hConfig = *pConfig;
		if (hConfig == NULL)
			continue;
		K22_D("Reading %s config", (pConfig - pK22Data->stReg.hConfig) ? "Per-App" : "Global");

		HKEY hDllExtra;
		if (K22_REG_OPEN_KEY(hConfig, "DllExtra", hDllExtra)) {
			if (!K22DataReadDllExtra(hDllExtra))
				return FALSE;
		}

		HKEY hDllRedirect;
		if (K22_REG_OPEN_KEY(hConfig, "DllRedirect", hDllRedirect)) {
			if (!K22DataReadDllRedirect(hDllRedirect))
				return FALSE;
		}

		HKEY hDllRewrite;
		if (K22_REG_OPEN_KEY(hConfig, "DllRewrite", hDllRewrite)) {
			if (!K22DataReadDllRewrite(hDllRewrite))
				return FALSE;
		}

		HKEY hWinVer;
		if (K22_REG_OPEN_KEY(hConfig, "WinVer", hWinVer)) {
			if (!K22DataReadWinVer(hWinVer))
				return FALSE;
		}
	}

	return TRUE;
}

static BOOL K22DataReadDllExtra(HKEY hDllExtra) {
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
			// K22_D(" - DLL Extra: will replace '%s'", pDllExtra->lpKey);
		}
		if (!K22StringDupFileName(szValue, cbValue - 1, &pDllExtra->lpTargetDll))
			return FALSE;
		K22_D(" - DLL Extra: setting '%s' (%s)", pDllExtra->lpKey, pDllExtra->lpTargetDll);
	}
	return TRUE;
}

static BOOL K22DataReadDllRedirect(HKEY hDllRedirect) {
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
			// K22_D(" - DLL Redirect: will replace %s", pDllRedirect->lpModuleName);
		}
		if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRedirect->lpTargetDll))
			return FALSE;
		K22_D(" - DLL Redirect: setting %s -> %s", pDllRedirect->lpSourceDll, pDllRedirect->lpTargetDll);
	}
	return TRUE;
}

static BOOL K22DataReadDllRewrite(HKEY hDllRewrite) {
	K22_REG_VARS();

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
			K22_D(" - DLL Rewrite: setting %s!* (missing) -> %s", pDllRewrite->lpSourceDll, pDllRewrite->lpDefaultDll);
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
				// K22_D(" - DLL Rewrite: will replace %s!%s", pDllRewrite->lpModuleName, pSymbol->lpSourceSymbol);
			}
			if (!K22StringDupDllTarget(szValue, cbValue - 1, &pSymbol->lpTargetDll, &pSymbol->lpTargetSymbol))
				return FALSE;
			K22_D(
				" - DLL Rewrite: setting %s!%s -> %s!%s",
				pDllRewrite->lpSourceDll,
				pSymbol->lpSourceSymbol,
				pSymbol->lpTargetDll,
				pSymbol->lpTargetSymbol ? pSymbol->lpTargetSymbol : pSymbol->lpSourceSymbol
			);
		}
	}
	return TRUE;
}

static VOID K22ParseWinVer(LPSTR lpWinVer, PK22_WIN_VER_ENTRY pWinVerEntry) {
	sscanf(lpWinVer, "%ld.%ld.%ld", &pWinVerEntry->dwMajor, &pWinVerEntry->dwMinor, &pWinVerEntry->dwBuild);
}

static BOOL K22DataReadWinVer(HKEY hWinVer) {
	K22_REG_VARS();

	if (K22_REG_READ_VALUE(hWinVer, "ModeFlags", &pK22Data->stDll.pWinVer->fMode, cbValue)) {
		K22_D(" - WinVer mode: %08lx", pK22Data->stDll.pWinVer->fMode);
	}

	if (K22_REG_READ_VALUE(hWinVer, NULL, szValue, cbValue)) {
		if (szValue[0] == '\0') {
			pK22Data->stDll.pWinVer->stDefault.dwMajor = NtCurrentPeb()->OSMajorVersion;
			pK22Data->stDll.pWinVer->stDefault.dwMinor = NtCurrentPeb()->OSMinorVersion;
			pK22Data->stDll.pWinVer->stDefault.dwBuild = NtCurrentPeb()->OSBuildNumber;
		} else {
			K22ParseWinVer(szValue, &pK22Data->stDll.pWinVer->stDefault);
		}
		K22_D(
			" - WinVer: setting %ld.%ld.%ld as default",
			pK22Data->stDll.pWinVer->stDefault.dwMajor,
			pK22Data->stDll.pWinVer->stDefault.dwMinor,
			pK22Data->stDll.pWinVer->stDefault.dwBuild
		);
	}

	K22_REG_ENUM_VALUE(hWinVer, szName, cbName, szValue, cbValue) {
		if (szName[0] == '\0')				  // skip Default value
			continue;
		if (strcmp(szName, "ModeFlags") == 0) // skip ModeFlags value
			continue;
		PK22_WIN_VER_ENTRY pWinVerEntry;
		K22_LL_FIND(
			pK22Data->stDll.pWinVer->pEntries,
			pWinVerEntry, // comparison
			strcmp(szName, pWinVerEntry->lpModuleName) == 0 || strcmp(szName, pWinVerEntry->lpModeName) == 0
		);
		if (pWinVerEntry == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stDll.pWinVer->pEntries, pWinVerEntry);
			if (strchr(szName, '.') != NULL) {
				if (!K22StringDup(szName, cbName, &pWinVerEntry->lpModuleName))
					return FALSE;
			} else {
				if (!K22StringDup(szName, cbName, &pWinVerEntry->lpModeName))
					return FALSE;
			}
		} else {
			/*K22_D(
				" - WinVer: will replace %s",
				pWinVerEntry->lpModuleName ? pWinVerEntry->lpModuleName : pWinVerEntry->lpModeName
			);*/
		}
		K22ParseWinVer(szValue, pWinVerEntry);
		K22_D(
			" - WinVer: setting %ld.%ld.%ld for %s",
			pWinVerEntry->dwMajor,
			pWinVerEntry->dwMinor,
			pWinVerEntry->dwBuild,
			pWinVerEntry->lpModuleName ? pWinVerEntry->lpModuleName : pWinVerEntry->lpModeName
		);
	}

	return TRUE;
}
