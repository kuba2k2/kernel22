// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

PK22_DATA pK22Data;

static BOOL K22DataDupString(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
static BOOL K22DataDupFileName(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);

BOOL K22DataInitialize(PIMAGE_K22_HEADER pK22Header) {
	if (pK22Data == NULL)
		K22_CALLOC(pK22Data);

	pK22Data->pK22Header = pK22Header;
	pK22Data->pNt		 = (PIMAGE_NT_HEADERS3264)((ULONG_PTR)pK22Data->lpProcessBase + pK22Data->pDosHeader->e_lfanew);
	pK22Data->pPeb		 = NtCurrentPeb();

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
		K22_D("Per-app configuration key opened");
	}

	return TRUE;
}

BOOL K22DataReadConfig() {
	DWORD dwError;
	TCHAR szName[256 + 1];
	TCHAR szValue[256 + 1];
	DWORD cbName;
	DWORD cbValue;

	DWORD dwIndex;

	K22_REG_REQUIRE_VALUE(pK22Data->stReg.hMain, "InstallDir", szValue, cbValue);
	pK22Data->stConfig.lpInstallDir = _strdup(szValue);
	pK22Data->stConfig.cbInstallDir = cbValue;

	for (PHKEY pConfig = pK22Data->stReg.hConfig; pConfig < pK22Data->stReg.hConfig + 2; pConfig++) {
		HKEY hConfig = *pConfig;
		if (hConfig == NULL)
			continue;
		K22_D("Reading %s config", (pConfig - pK22Data->stReg.hConfig) ? "Per-App" : "Global");

		HKEY hDllExtra;
		if (K22_REG_OPEN_KEY(hConfig, "DllExtra", hDllExtra)) {
			K22_REG_ENUM_VALUE(hDllExtra, szName, cbName, szValue, cbValue) {
				PK22_DLL_EXTRA pDllExtra;
				K22_LL_FIND(pK22Data->stConfig.pDllExtra, pDllExtra, strcmp(szName, pDllExtra->lpKey) == 0);
				if (pDllExtra == NULL) {
					K22_LL_ALLOC_APPEND(pK22Data->stConfig.pDllExtra, pDllExtra);
					if (!K22DataDupString(szName, cbName, &pDllExtra->lpKey))
						return FALSE;
				} else {
					K22_D(" - DLL Extra: removing %s", pDllExtra->lpFileName);
				}
				if (!K22DataDupFileName(szValue, cbValue - 1, &pDllExtra->lpFileName))
					return FALSE;
				K22_D(" - DLL Extra: adding %s", pDllExtra->lpFileName);
			}
		}
	}

	return TRUE;
}

static BOOL K22DataDupString(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput) {
	if (*ppOutput)
		free(*ppOutput);
	cchInput++; // count the NULL terminator
	K22_MALLOC_LENGTH(*ppOutput, cchInput);
	memcpy(*ppOutput, lpInput, cchInput);
	return TRUE;
}

static BOOL K22DataDupFileName(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput) {
	if (lpInput[0] != '@')
		return K22DataDupString(lpInput, cchInput, ppOutput);
	lpInput++; // skip the @ character; cchInput now accounts for the NULL terminator
	DWORD cchInstallDir = pK22Data->stConfig.cbInstallDir - 1;
	K22_MALLOC_LENGTH(*ppOutput, cchInstallDir + cchInput);
	memcpy(*ppOutput, pK22Data->stConfig.lpInstallDir, cchInstallDir);
	memcpy(*ppOutput + cchInstallDir, lpInput, cchInput);
	return TRUE;
}
