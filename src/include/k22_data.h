// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#pragma once

#include "kernel22.h"

#include "k22_extern.h"
#include "k22_types.h"

typedef struct K22_DATA *PK22_DATA;
typedef struct K22_MODULE_DATA *PK22_MODULE_DATA;
typedef struct K22_DLL_EXTRA *PK22_DLL_EXTRA;
typedef struct K22_DLL_REDIRECT *PK22_DLL_REDIRECT;
typedef struct K22_DLL_REWRITE *PK22_DLL_REWRITE;
typedef struct K22_WIN_VER *PK22_WIN_VER;

#if K22_CORE
extern PK22_DATA pK22Data;
#endif

// Runtime per-process data structure
typedef struct K22_DATA {
	union {
		LPVOID lpProcessBase;
		PIMAGE_DOS_HEADER pDosHeader;
		PIMAGE_K22_HEADER pK22Header;
	};

	PIMAGE_NT_HEADERS3264 pNt;
	LPSTR lpProcessDir;
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
		BOOL bDebugImportResolver;
	} stConfig;

	struct {
		PK22_DLL_EXTRA pDllExtra;
		PK22_DLL_REDIRECT pDllRedirect;
		PK22_DLL_REWRITE pDllRewrite;
		PK22_WIN_VER pWinVer;
	} stDll;
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

// DllExtra

typedef struct K22_DLL_EXTRA {
	LPSTR lpKey;	   // arbitrary name of this entry
	LPSTR lpTargetDll; // name of DLL to load
	HINSTANCE hModule; // handle to lpTargetDll
	struct K22_DLL_EXTRA *pPrev;
	struct K22_DLL_EXTRA *pNext;
} K22_DLL_EXTRA;

// DllRedirect

typedef struct K22_DLL_REDIRECT {
	LPSTR lpSourceDll; // source DLL
	LPSTR lpTargetDll; // target DLL
	HINSTANCE hModule; // handle to lpTargetDll
	struct K22_DLL_REDIRECT *pPrev;
	struct K22_DLL_REDIRECT *pNext;
} K22_DLL_REDIRECT;

// DllRewrite

typedef struct K22_DLL_REWRITE_SYMBOL {
	LPSTR lpSourceSymbol; // source symbol to match
	LPSTR lpTargetDll;	  // target DLL
	LPSTR lpTargetSymbol; // symbol to redirect to
	PVOID pProc;		  // pointer to target function
	struct K22_DLL_REWRITE_SYMBOL *pPrev;
	struct K22_DLL_REWRITE_SYMBOL *pNext;
} K22_DLL_REWRITE_SYMBOL, *PK22_DLL_REWRITE_SYMBOL;

typedef struct K22_DLL_REWRITE {
	LPSTR lpSourceDll;				  // source DLL
	LPSTR lpDefaultDll;				  // optional, target DLL for missing symbols
	LPSTR lpCatchAllDll;			  // optional, target DLL for all symbols
	HINSTANCE hDefault;				  // optional, handle to lpDefaultDll
	HINSTANCE hCatchAll;			  // optional, handle to lpCatchAllDll
	PK22_DLL_REWRITE_SYMBOL pSymbols; // list of specific symbols to rewrite
	struct K22_DLL_REWRITE *pPrev;
	struct K22_DLL_REWRITE *pNext;
} K22_DLL_REWRITE;

// DllHook
// TBD

// WinVer

typedef struct K22_WIN_VER_ENTRY {
	LPSTR lpModuleName; // module name to match
	LPSTR lpModeName;	// spoofing mode to match
	DWORD dwMajor;		// major version number
	DWORD dwMinor;		// minor version number
	DWORD dwBuild;		// build number
	struct K22_WIN_VER_ENTRY *pPrev;
	struct K22_WIN_VER_ENTRY *pNext;
} K22_WIN_VER_ENTRY, *PK22_WIN_VER_ENTRY;

typedef struct K22_WIN_VER {
	DWORD fMode;				 // enabled spoofing modes
	K22_WIN_VER_ENTRY stDefault; // default version to apply
	PK22_WIN_VER_ENTRY pEntries; // per-module/per-mode version spoofing
} K22_WIN_VER;

// BinPatch
// TBD
