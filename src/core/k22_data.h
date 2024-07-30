// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#pragma once

#include "kernel22.h"

typedef struct K22_DATA *PK22_DATA;
typedef struct K22_MODULE_DATA *PK22_MODULE_DATA;

// Runtime per-process data structure
typedef struct K22_DATA {
	union {
		LPVOID lpProcessBase;
		PIMAGE_DOS_HEADER pDosHeader;
		PIMAGE_K22_HEADER pK22Header;
	};

	PIMAGE_NT_HEADERS3264 pNt;
	LPSTR lpProcessPath;
	LPSTR lpProcessName;
	BOOL fIs64Bit;
	BOOL fDelayDllInit;

	struct {
		HKEY hMain;
		HKEY hConfig[2];
	} stReg;

	struct {
		LPSTR lpInstallDir;
		SIZE_T cchInstallDir;
		DWORD dwDllNotificationMode;
		PK22_DLL_EXTRA pDllExtra;
		PK22_DLL_REDIRECT pDllRedirect;
		PK22_DLL_REWRITE pDllRewrite;
		PK22_WIN_VER pWinVer;
	} stConfig;
} K22_DATA;

// Runtime per-module data structure
typedef struct K22_MODULE_DATA {
	union {
		LPVOID lpModuleBase;
		PIMAGE_DOS_HEADER pDosHeader;
	};

	PIMAGE_NT_HEADERS3264 pNt;
	PLDR_DATA_TABLE_ENTRY pLdrEntry;
	LPSTR lpModulePath;
	LPSTR lpModuleName;
	BOOL fIsProcess;

	PDLL_INIT_ROUTINE lpDelayedInitRoutine;
} K22_MODULE_DATA;

#if K22_CORE
extern PK22_DATA pK22Data;
#endif
