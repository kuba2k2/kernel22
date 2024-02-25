// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#pragma once

#include "kernel22.h"

// DllExtra

typedef struct K22_DLL_EXTRA {
	LPSTR lpKey;	  // arbitrary name of this entry
	LPSTR lpFileName; // name of DLL to load
	struct K22_DLL_EXTRA *pPrev;
	struct K22_DLL_EXTRA *pNext;
} K22_DLL_EXTRA, *PK22_DLL_EXTRA;

// DllRedirect

typedef struct K22_DLL_REDIRECT {
	LPSTR lpModuleName; // source DLL
	LPSTR lpFileName;	// target DLL
	struct K22_DLL_REDIRECT *pPrev;
	struct K22_DLL_REDIRECT *pNext;
} K22_DLL_REDIRECT, *PK22_DLL_REDIRECT;

// DllRewrite

typedef struct K22_DLL_REWRITE_SYMBOL {
	LPSTR lpSourceSymbol; // source symbol to match
	LPSTR lpFileName;	  // target DLL
	LPSTR lpTargetSymbol; // optional, symbol to redirect to (same as source if unset)
	struct K22_DLL_REWRITE_SYMBOL *pPrev;
	struct K22_DLL_REWRITE_SYMBOL *pNext;
} K22_DLL_REWRITE_SYMBOL, *PK22_DLL_REWRITE_SYMBOL;

typedef struct K22_DLL_REWRITE {
	LPSTR lpModuleName;				  // source DLL
	LPSTR pDefault;					  // optional, target DLL for missing symbols
	LPSTR pCatchAll;				  // optional, target DLL for all symbols
	PK22_DLL_REWRITE_SYMBOL pSymbols; // list of specific symbols to rewrite
	struct K22_DLL_REWRITE *pPrev;
	struct K22_DLL_REWRITE *pNext;
} K22_DLL_REWRITE, *PK22_DLL_REWRITE;

// DllHook
// TBD

// WinVer

typedef struct K22_WIN_VER_ENTRY {
	LPSTR lpModuleName; // mode name to match
	LPSTR lpModeName;	// mode name to match
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
} K22_WIN_VER, *PK22_WIN_VER;

// BinPatch
// TBD

typedef struct {
	union {
		LPVOID lpProcessBase;
		PIMAGE_DOS_HEADER pDosHeader;
		PIMAGE_K22_HEADER pK22Header;
	};

	PIMAGE_NT_HEADERS3264 pNt;

	PPEB pPeb;
	LPSTR lpProcessPath;
	LPSTR lpProcessName;

	struct {
		HKEY hMain;
		HKEY hConfig[2];
	} stReg;

	struct {
		LPSTR lpInstallDir;
		SIZE_T cbInstallDir;
		PK22_DLL_EXTRA pDllExtra;
		PK22_DLL_REDIRECT pDllRedirect;
		PK22_DLL_REWRITE pDllRewrite;
		PK22_WIN_VER pWinVer;
	} stConfig;
} K22_DATA, *PK22_DATA;

extern PK22_DATA pK22Data;
