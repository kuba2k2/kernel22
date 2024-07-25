// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#pragma once

#include "kernel22.h"

#include "k22_config.h"

typedef struct {
	union {
		LPVOID lpProcessBase;
		PIMAGE_DOS_HEADER pDosHeader;
		PIMAGE_K22_HEADER pK22Header;
	};

	PIMAGE_NT_HEADERS3264 pNt;

	PPEB pPeb;
	BOOL fIs64Bit;
	LPSTR lpProcessPath;
	LPSTR lpProcessName;

	struct {
		HKEY hMain;
		HKEY hConfig[2];
	} stReg;

	struct {
		LPSTR lpInstallDir;
		SIZE_T cchInstallDir;
		PK22_DLL_EXTRA pDllExtra;
		PK22_DLL_REDIRECT pDllRedirect;
		PK22_DLL_REWRITE pDllRewrite;
		PK22_WIN_VER pWinVer;
	} stConfig;
} K22_DATA, *PK22_DATA;

typedef struct {
	union {
		LPVOID lpModuleBase;
		PIMAGE_DOS_HEADER pDosHeader;
	};

	PIMAGE_NT_HEADERS3264 pNt;

	LPSTR lpModulePath;
	LPSTR lpModuleName;
} K22_MODULE_DATA, *PK22_MODULE_DATA;

#if K22_CORE
extern PK22_DATA pK22Data;
#endif
