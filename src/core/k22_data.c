// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

PK22_DATA pK22Data;

static LPSTR K22DataDupFileName(LPSTR lpInput, DWORD cbInput);

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
	PVOID ppNext;

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
			PK22_DLL_EXTRA pDllExtra;
			K22_LL_END(PK22_DLL_EXTRA, pK22Data->stConfig.pDllExtra, pDllExtra, ppNext);

			K22_REG_ENUM_VALUE(hDllExtra, szName, cbName, szValue, cbValue) {
				K22_LL_APPEND(PK22_DLL_EXTRA, pDllExtra, ppNext);
				pDllExtra->lpFileName = K22DataDupFileName(szValue, cbValue);
				K22_D(" - DLL Extra: %s", pDllExtra->lpFileName);
			}
		}
	}

	return TRUE;
}

static LPSTR K22DataDupFileName(LPSTR lpInput, DWORD cbInput) {
	if (lpInput[0] != '@')
		return _strdup(lpInput);
	LPSTR lpOutput;
	DWORD cchInstallDir = pK22Data->stConfig.cbInstallDir - 1;
	cbInput--;
	lpInput++;

	K22_MALLOC_LENGTH(lpOutput, cchInstallDir + cbInput);
	memcpy(lpOutput, pK22Data->stConfig.lpInstallDir, cchInstallDir);
	memcpy(lpOutput + cchInstallDir, lpInput, cbInput);

	return lpOutput;
}
