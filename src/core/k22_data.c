// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

PK22_DATA pK22Data;

static BOOL K22DataReadDllExtra(HKEY hDllExtra);
static BOOL K22DataReadDllRedirect(HKEY hDllRedirect);
static BOOL K22DataReadDllRewrite(HKEY hDllRewrite);
static BOOL K22DataReadWinVer(HKEY hWinVer);

BOOL K22DataInitialize(PIMAGE_K22_HEADER pK22Header) {
	if (pK22Data == NULL)
		K22_CALLOC(pK22Data);

	pK22Data->pK22Header = pK22Header;
	pK22Data->pNt		 = (PIMAGE_NT_HEADERS3264)((ULONG_PTR)pK22Data->lpProcessBase + pK22Data->pDosHeader->e_lfanew);
	pK22Data->pPeb		 = NtCurrentPeb();
	pK22Data->fIs64Bit	 = pK22Data->pNt->stFile.Machine == IMAGE_FILE_MACHINE_AMD64;

	K22_MALLOC_LENGTH(pK22Data->lpProcessPath, MAX_PATH + 1);
	GetModuleFileName(NULL, pK22Data->lpProcessPath, MAX_PATH + 1);
	LPSTR lpProcessName = pK22Data->lpProcessName = pK22Data->lpProcessPath;
	while (*lpProcessName) {
		if (*lpProcessName++ == '\\')
			pK22Data->lpProcessName = lpProcessName;
	}

	// open registry keys
	K22_REG_REQUIRE_KEY(HKEY_LOCAL_MACHINE, K22_REG_KEY_PATH, pK22Data->stReg.hMain);
	K22_REG_REQUIRE_KEY(pK22Data->stReg.hMain, "Global", pK22Data->stReg.hConfig[0]);
	K22_REG_REQUIRE_KEY(pK22Data->stReg.hMain, "PerApp", pK22Data->stReg.hConfig[1]);
	K22_D("Opened main registry keys");

	// open per-app registry key if present
	if (RegOpenKeyEx(
			pK22Data->stReg.hConfig[1],
			pK22Data->lpProcessName,
			0,
			K22_REG_ACCESS,
			&pK22Data->stReg.hConfig[1]
		) != ERROR_SUCCESS) {
		K22_D("Per-app configuration key not found");
		pK22Data->stReg.hConfig[1] = NULL;
	} else {
		K22_D("Per-app configuration key found");
	}

	return TRUE;
}

BOOL K22DataReadConfig() {
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
			pK22Data->stConfig.pDllExtra,
			pDllExtra,
			// comparison
			strcmp(szName, pDllExtra->lpKey) == 0
		);
		if (pDllExtra == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stConfig.pDllExtra, pDllExtra);
			if (!K22StringDup(szName, cbName, &pDllExtra->lpKey))
				return FALSE;
		} else {
			K22_D(" - DLL Extra: removing %s", pDllExtra->lpFileName);
		}
		if (!K22StringDupFileName(szValue, cbValue - 1, &pDllExtra->lpFileName))
			return FALSE;
		K22_D(" - DLL Extra: adding %s", pDllExtra->lpFileName);
	}
	return TRUE;
}

static BOOL K22DataReadDllRedirect(HKEY hDllRedirect) {
	K22_REG_VARS();

	K22_REG_ENUM_VALUE(hDllRedirect, szName, cbName, szValue, cbValue) {
		PK22_DLL_REDIRECT pDllRedirect;
		K22_LL_FIND(
			pK22Data->stConfig.pDllRedirect,
			pDllRedirect,
			// comparison
			strcmp(szName, pDllRedirect->lpModuleName) == 0
		);
		if (pDllRedirect == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stConfig.pDllRedirect, pDllRedirect);
			if (!K22StringDup(szName, cbName, &pDllRedirect->lpModuleName))
				return FALSE;
		} else {
			K22_D(" - DLL Redirect: removing %s -> %s", pDllRedirect->lpModuleName, pDllRedirect->lpFileName);
		}
		if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRedirect->lpFileName))
			return FALSE;
		K22_D(" - DLL Redirect: adding %s -> %s", pDllRedirect->lpModuleName, pDllRedirect->lpFileName);
	}
	return TRUE;
}

static BOOL K22DataReadDllRewrite(HKEY hDllRewrite) {
	K22_REG_VARS();

	K22_REG_ENUM_KEY(hDllRewrite, szName, cbName) {
		PK22_DLL_REWRITE pDllRewrite;
		K22_LL_FIND(
			pK22Data->stConfig.pDllRewrite,
			pDllRewrite,
			// comparison
			strcmp(szName, pDllRewrite->lpModuleName) == 0
		);
		if (pDllRewrite == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stConfig.pDllRewrite, pDllRewrite);
			if (!K22StringDup(szName, cbName, &pDllRewrite->lpModuleName))
				return FALSE;
		}
		HKEY hDllRewriteItem;
		K22_REG_REQUIRE_KEY(hDllRewrite, szName, hDllRewriteItem);
		if (K22_REG_READ_VALUE(hDllRewriteItem, NULL, szValue, cbValue)) {
			if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRewrite->pDefault))
				return FALSE;
		}
		if (K22_REG_READ_VALUE(hDllRewriteItem, "*", szValue, cbValue)) {
			if (!K22StringDupFileName(szValue, cbValue - 1, &pDllRewrite->pCatchAll))
				return FALSE;
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
				K22_D(
					" - DLL Rewrite: removing %s!%s -> %s!%s",
					pDllRewrite->lpModuleName,
					pSymbol->lpSourceSymbol,
					pSymbol->lpFileName,
					pSymbol->lpTargetSymbol ? pSymbol->lpTargetSymbol : pSymbol->lpSourceSymbol
				);
			}
			if (!K22StringDupDllTarget(szValue, cbValue - 1, &pSymbol->lpFileName, &pSymbol->lpTargetSymbol))
				return FALSE;
			K22_D(
				" - DLL Rewrite: adding %s!%s -> %s!%s",
				pDllRewrite->lpModuleName,
				pSymbol->lpSourceSymbol,
				pSymbol->lpFileName,
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

	if (pK22Data->stConfig.pWinVer == NULL) {
		K22_CALLOC(pK22Data->stConfig.pWinVer);
		pK22Data->stConfig.pWinVer->stDefault.dwMajor = NtCurrentPeb()->OSMajorVersion;
		pK22Data->stConfig.pWinVer->stDefault.dwMinor = NtCurrentPeb()->OSMinorVersion;
		pK22Data->stConfig.pWinVer->stDefault.dwBuild = NtCurrentPeb()->OSBuildNumber;
	}

	if (K22_REG_READ_VALUE(hWinVer, "ModeFlags", &pK22Data->stConfig.pWinVer->fMode, cbValue)) {
		K22_D(" - WinVer mode: %08lx", pK22Data->stConfig.pWinVer->fMode);
	}

	if (K22_REG_READ_VALUE(hWinVer, NULL, szValue, cbValue)) {
		K22ParseWinVer(szValue, &pK22Data->stConfig.pWinVer->stDefault);
		K22_D(
			" - WinVer default: %ld.%ld.%ld",
			pK22Data->stConfig.pWinVer->stDefault.dwMajor,
			pK22Data->stConfig.pWinVer->stDefault.dwMinor,
			pK22Data->stConfig.pWinVer->stDefault.dwBuild
		);
	}

	K22_REG_ENUM_VALUE(hWinVer, szName, cbName, szValue, cbValue) {
		if (szName[0] == '\0')				  // skip Default value
			continue;
		if (strcmp(szName, "ModeFlags") == 0) // skip ModeFlags value
			continue;
		PK22_WIN_VER_ENTRY pWinVerEntry;
		K22_LL_FIND(
			pK22Data->stConfig.pWinVer->pEntries,
			pWinVerEntry, // comparison
			strcmp(szName, pWinVerEntry->lpModuleName) == 0 || strcmp(szName, pWinVerEntry->lpModeName) == 0
		);
		if (pWinVerEntry == NULL) {
			K22_LL_ALLOC_APPEND(pK22Data->stConfig.pWinVer->pEntries, pWinVerEntry);
			if (strchr(szName, '.') != NULL) {
				if (!K22StringDup(szName, cbName, &pWinVerEntry->lpModuleName))
					return FALSE;
			} else {
				if (!K22StringDup(szName, cbName, &pWinVerEntry->lpModeName))
					return FALSE;
			}
		} else {
			K22_D(
				" - WinVer: removing %s = %ld.%ld.%ld",
				pWinVerEntry->lpModuleName ? pWinVerEntry->lpModuleName : pWinVerEntry->lpModeName,
				pWinVerEntry->dwMajor,
				pWinVerEntry->dwMinor,
				pWinVerEntry->dwBuild
			);
		}
		K22ParseWinVer(szValue, pWinVerEntry);
		K22_D(
			" - WinVer: adding %s = %ld.%ld.%ld",
			pWinVerEntry->lpModuleName ? pWinVerEntry->lpModuleName : pWinVerEntry->lpModeName,
			pWinVerEntry->dwMajor,
			pWinVerEntry->dwMinor,
			pWinVerEntry->dwBuild
		);
	}

	return TRUE;
}
