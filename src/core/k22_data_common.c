// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

static BOOL K22DataInitialize(LPVOID lpImageBase);
static BOOL K22DataInitializeModule(LPVOID lpImageBase);

PK22_DATA pK22Data;

PK22_DATA K22DataGet() {
	if (pK22Data == NULL && !K22DataInitialize(GetModuleHandle(NULL)))
		return NULL;
	return pK22Data;
}

PK22_MODULE_DATA K22DataGetModule(LPVOID lpImageBase) {
	PIMAGE_K22_HEADER pK22Header = K22_DOS_HDR_DATA(lpImageBase);
	if (pK22Header->dwCoreMagic != K22_CORE_MAGIC && !K22DataInitializeModule(lpImageBase))
		return NULL;
	return pK22Header->lpModuleData;
}

static BOOL K22DataInitialize(LPVOID lpImageBase) {
	if (pK22Data == NULL)
		K22_CALLOC(pK22Data);

	pK22Data->lpProcessBase = lpImageBase;
	pK22Data->pNt			= RVA(pK22Data->pDosHeader->e_lfanew);
	pK22Data->fIs64Bit		= pK22Data->pNt->stFile.Machine == IMAGE_FILE_MACHINE_AMD64;

	K22_MALLOC_LENGTH(pK22Data->lpProcessDir, MAX_PATH + 1);
	GetModuleFileName(NULL, pK22Data->lpProcessDir, MAX_PATH + 1);
	LPSTR lpProcessName = pK22Data->lpProcessName = pK22Data->lpProcessDir;
	while (*lpProcessName) {
		if (*lpProcessName++ == '\\')
			pK22Data->lpProcessName = lpProcessName;
	}
	pK22Data->lpProcessName[-1] = '\0';

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

	TCHAR szInstallDir[MAX_PATH + 1];
	DWORD cbInstallDir = sizeof(szInstallDir);
	if (!(cbInstallDir = K22ConfigReadValueGlobal("InstallDir", szInstallDir, cbInstallDir)))
		RETURN_K22_F_ERR("Couldn't read InstallDir from registry");
	cbInstallDir--; // skip the NULL terminator
	if (szInstallDir[cbInstallDir - 1] != '\\')
		szInstallDir[cbInstallDir++] = '\\';
	strcpy(szInstallDir + cbInstallDir, pK22Data->fIs64Bit ? "DLL_64\\" : "DLL_32\\");
	cbInstallDir += 7;
	pK22Data->stConfig.lpInstallDir	 = _strdup(szInstallDir);
	pK22Data->stConfig.cchInstallDir = cbInstallDir;

	K22ConfigReadValueGlobal("LogLevel", &pK22Data->stConfig.dwLogLevel, sizeof(DWORD));
	K22ConfigReadValueGlobal("DllNotificationMode", &pK22Data->stConfig.dwDllNotificationMode, sizeof(DWORD));
	K22ConfigReadValueGlobal("DebugImportResolver", &pK22Data->stConfig.bDebugImportResolver, sizeof(BOOL));

	if (!K22ConfigReadKey("DllExtra", K22ConfigParseDllExtra))
		return FALSE;
	if (!K22ConfigReadKey("DllApiSet", K22ConfigParseDllApiSet))
		return FALSE;
	if (!K22ConfigReadKey("DllRedirect", K22ConfigParseDllRedirect))
		return FALSE;
	if (!K22ConfigReadKey("DllRewrite", K22ConfigParseDllRewrite))
		return FALSE;

	return TRUE;
}

static BOOL K22DataInitializeModule(LPVOID lpImageBase) {
	PIMAGE_K22_HEADER pK22Header = K22_DOS_HDR_DATA(lpImageBase);

	PK22_MODULE_DATA pK22ModuleData;
	K22_CALLOC(pK22ModuleData);

	pK22ModuleData->lpModuleBase = lpImageBase;
	pK22ModuleData->pNt			 = RVA(pK22ModuleData->pDosHeader->e_lfanew);
	pK22ModuleData->fIsProcess	 = lpImageBase == pK22Data->lpProcessBase;

	// find the module path and base name
	K22_LDR_ENUM(pLdrEntry, InLoadOrderModuleList, InLoadOrderLinks) {
		if (pLdrEntry->DllBase != lpImageBase)
			continue;
		ANSI_STRING stName;
		// store data table entry
		pK22ModuleData->pLdrEntry = pLdrEntry;
		// convert to ANSI, then to lowercase and store in module data
		RtlUnicodeStringToAnsiString(&stName, &pLdrEntry->FullDllName, TRUE);
		_strlwr(stName.Buffer);
		pK22ModuleData->lpModulePath = stName.Buffer;
		// same for module name
		RtlUnicodeStringToAnsiString(&stName, &pLdrEntry->BaseDllName, TRUE);
		_strlwr(stName.Buffer);
		pK22ModuleData->lpModuleName = stName.Buffer;
		break;
	}

	K22WithUnlocked(*pK22Header) {
		pK22Header->dwCoreMagic	 = K22_CORE_MAGIC;
		pK22Header->lpModuleData = pK22ModuleData;
	}
	return TRUE;
}
