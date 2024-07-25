// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#pragma once

#include "kernel22.h"

#include "MinHook.h"

typedef struct K22_DATA *PK22_DATA;
typedef struct K22_MODULE_DATA *PK22_MODULE_DATA;

// k22_config.c
BOOL K22ConfigRead();
// k22_core.c
BOOL K22CoreMain(PIMAGE_K22_HEADER pK22Header);
// k22_data.c
PK22_DATA K22DataGet();
PK22_MODULE_DATA K22DataGetModule(LPVOID lpImageBase);
BOOL K22DataInitialize(LPVOID lpImageBase);
BOOL K22DataInitializeModule(LPVOID lpImageBase);
// k22_file.c
BOOL K22StringDup(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
BOOL K22StringDupFileName(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
BOOL K22StringDupDllTarget(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput, LPSTR *ppSymbol);
// k22_import.c
BOOL K22ImportTableRestore(LPVOID lpImageBase);
BOOL K22ProcessImports(LPVOID lpImageBase);
// k22_resolve.c
PVOID K22ResolveSymbol(LPCSTR lpDllName, LPCSTR lpSymbolName, ULONG_PTR ulSymbolOrdinal);

// Runtime per-process data structure
typedef struct K22_DATA {
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
} K22_DATA;

// Runtime per-module data structure
typedef struct K22_MODULE_DATA {
	union {
		LPVOID lpModuleBase;
		PIMAGE_DOS_HEADER pDosHeader;
	};

	PIMAGE_NT_HEADERS3264 pNt;

	LPSTR lpModulePath;
	LPSTR lpModuleName;
} K22_MODULE_DATA;

#if K22_CORE
extern PK22_DATA pK22Data;
#endif
