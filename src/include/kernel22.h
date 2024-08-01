// Copyright (c) Kuba Szczodrzyński 2024-2-12.

#pragma once

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <Windows.h>
#include <processthreadsapi.h>
#include <psapi.h>
#include <strsafe.h>

#include "ntdll.h"
#include "utlist.h"

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2 * !!(condition)]))

#if K22_CORE
// core exports public functions
#define K22_CORE_PROC __declspec(dllexport)
#elif K22_VERIFIER
// verifier has its own copy
#define K22_CORE_PROC
#else
// everything else imports from core
#define K22_CORE_PROC __declspec(dllimport)
#endif

#include "k22_options.h"

#include "k22_extern.h"
#include "k22_logger.h"
#include "k22_macros.h"
#include "k22_types.h"

#if K22_LOADER
#include "k22_debugger.h"
#include "k22_loader.h"
#endif

#if K22_CORE
#include "MinHook.h"
#include "k22_data.h"
#endif

#if K22_VERIFIER
#define K22LogWrite(...)
#endif

/* Public core functions */

// k22_hexdump.c
K22_CORE_PROC VOID K22HexDump(CONST BYTE *pBuf, SIZE_T cbLength, ULONGLONG ullOffset);
K22_CORE_PROC VOID K22HexDumpProcess(HANDLE hProcess, LPCVOID lpAddress, SIZE_T cbLength);
// k22_patch.c
K22_CORE_PROC BOOL K22PatchImportTable(BYTE bSource, LPVOID lpImageBase);
K22_CORE_PROC BOOL K22PatchImportTableProcess(BYTE bSource, HANDLE hProcess, LPVOID lpImageBase);
K22_CORE_PROC BOOL K22PatchImportTableFile(BYTE bSource, HANDLE hFile);
K22_CORE_PROC BOOL K22ClearBoundImportTable(LPVOID lpImageBase);
// k22_process.c
K22_CORE_PROC BOOL K22ProcessPatchVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor);
K22_CORE_PROC BOOL K22ProcessReadPeb(HANDLE hProcess, PPEB pPeb);
K22_CORE_PROC BOOL K22ProcessWritePeb(HANDLE hProcess, PPEB pPeb);

/* Private core functions */

#if K22_CORE
// k22_core.c
BOOL K22CoreMain(PIMAGE_K22_HEADER pK22Header, LPVOID lpContext);
// k22_data_init.c
PK22_DATA K22DataGet();
PK22_MODULE_DATA K22DataGetModule(LPVOID lpImageBase);
BOOL K22DataInitialize(LPVOID lpImageBase);
BOOL K22DataInitializeModule(LPVOID lpImageBase);
// k22_data_registry.c
BOOL K22DataReadRegistry();
// k22_data_utils.c
BOOL K22StringDup(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
BOOL K22StringDupFileName(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
BOOL K22StringDupDllTarget(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput, LPSTR *ppSymbol);
// k22_import.c
BOOL K22ProcessImports(LPVOID lpImageBase);
BOOL K22CallInitRoutines(LPVOID lpContext);
BOOL K22DummyEntryPoint(HANDLE hDll, DWORD dwReason, LPVOID lpContext);
// k22_resolve.c
PVOID K22ResolveSymbol(LPCSTR lpModuleName, LPCSTR lpSymbolName, WORD wSymbolOrdinal);
// k22_utils.c
VOID K22DebugPrintModules();
BOOL K22DebugDumpModules(LPCSTR lpOutputDir, LPVOID lpImageBase);
PLDR_DATA_TABLE_ENTRY K22GetLdrEntry(LPVOID lpImageBase);
#endif
