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
#define K22_DLL_EXPORT			__declspec(dllexport)

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

#include "k22_data.h"
#include "k22_extern.h"
#include "k22_hook.h"
#include "k22_logger.h"
#include "k22_macros.h"
#include "k22_types.h"

#if K22_LOADER
#include "k22_debugger.h"
#include "k22_loader.h"
#endif

#if K22_CORE
#include "MinHook.h"
#endif

#if K22_VERIFIER
#define K22LogWrite(...)
#endif

/* Common functions (core + verifier) */

// k22_patch.c / k22_patch_impl.c
K22_CORE_PROC BOOL K22PatchImportTable(BYTE bSource, LPVOID lpImageBase);
K22_CORE_PROC BOOL K22PatchImportTableProcess(BYTE bSource, HANDLE hProcess, LPVOID lpImageBase);
K22_CORE_PROC BOOL K22PatchImportTableFile(BYTE bSource, HANDLE hFile);
K22_CORE_PROC BOOL K22ClearBoundImportTable(LPVOID lpImageBase);

/* Public core functions */

// k22_data_common.c
K22_CORE_PROC PK22_DATA K22DataGet();
K22_CORE_PROC PK22_MODULE_DATA K22DataGetModule(LPVOID lpImageBase);
// k22_data_registry.c
K22_CORE_PROC DWORD K22ConfigReadValueGlobal(LPCSTR lpName, PVOID pValue, DWORD cbValue);
K22_CORE_PROC DWORD K22ConfigReadValue(LPCSTR lpName, PVOID pValue, DWORD cbValue);
K22_CORE_PROC BOOL K22ConfigReadKey(LPCSTR lpName, BOOL (*pProc)(HKEY));
// k22_data_utils.c
K22_CORE_PROC BOOL K22StringDup(LPCSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
K22_CORE_PROC BOOL K22StringDupFileName(LPCSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
K22_CORE_PROC BOOL K22StringDupDllTarget(LPCSTR lpInput, DWORD cchInput, LPSTR *ppOutput, LPSTR *ppSymbol);
K22_CORE_PROC BOOL K22PathMatches(LPCSTR lpPath, LPCSTR lpPattern);
K22_CORE_PROC BOOL K22PathIsFile(LPCSTR lpPath);
K22_CORE_PROC BOOL K22PathIsFileEx(LPSTR lpDirectory, DWORD cchDirectory, LPCSTR lpName);
// k22_dll_ldrapi.c
K22_HOOK_REAL_DEF(
	NTSTATUS,
	LdrLoadDll,
	(PCWSTR pDllPath, PULONG pDllCharacteristics, PUNICODE_STRING pDllName, PVOID *ppDllHandle)
);
K22_HOOK_REAL_DEF(
	NTSTATUS,
	LdrGetDllHandleEx,
	(ULONG ulFlags, PCWSTR pDllPath, PULONG pDllCharacteristics, PUNICODE_STRING pDllName, PVOID *ppDllHandle)
);
K22_HOOK_REAL_DEF(
	NTSTATUS,
	LdrGetProcedureAddress,
	(PVOID pDllHandle, PANSI_STRING pProcedureName, ULONG ulProcedureNumber, PVOID *ppProcedureAddress)
);
// k22_hexdump.c
K22_CORE_PROC VOID K22HexDump(CONST BYTE *pBuf, SIZE_T cbLength, ULONGLONG ullOffset);
K22_CORE_PROC VOID K22HexDumpProcess(HANDLE hProcess, LPCVOID lpAddress, SIZE_T cbLength);
// k22_process.c
K22_CORE_PROC BOOL K22ProcessPatchVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor);
K22_CORE_PROC BOOL K22ProcessReadPeb(HANDLE hProcess, PPEB pPeb);
K22_CORE_PROC BOOL K22ProcessWritePeb(HANDLE hProcess, PPEB pPeb);
// k22_utils.c
K22_CORE_PROC VOID K22DebugPrintModules();
K22_CORE_PROC BOOL K22DebugDumpModules(LPCSTR lpOutputDir, LPVOID lpImageBase);
K22_CORE_PROC PLDR_DATA_TABLE_ENTRY K22GetLdrEntry(LPVOID lpImageBase);
K22_CORE_PROC BOOL K22HookCreate(LPVOID pProc, LPVOID pHook, LPVOID *ppReal);
K22_CORE_PROC BOOL K22HookRemove(LPVOID pProc);

/* Private core functions */

#if K22_CORE
// k22_core.c
BOOL K22CoreMain(PIMAGE_K22_HEADER pK22Header, LPVOID lpContext);
// k22_data_config.c
BOOL K22ConfigParseDllExtra(HKEY hDllExtra);
BOOL K22ConfigParseDllApiSet(HKEY hDllApiSet);
BOOL K22ConfigParseDllRedirect(HKEY hDllRedirect);
BOOL K22ConfigParseDllRewrite(HKEY hDllRewrite);
// k22_dll_entry_find.c
PK22_DLL_API_SET K22FindDllApiSet(LPCSTR lpModuleName, LPCSTR lpSymbolName);
PK22_DLL_REDIRECT K22FindDllRedirect(LPCSTR lpModuleName);
PK22_DLL_REWRITE K22FindDllRewrite(LPCSTR lpModuleName);
PK22_DLL_REWRITE_SYMBOL K22FindDllRewriteSymbol(PK22_DLL_REWRITE pDllRewrite, LPCSTR lpSymbolName);
// k22_dll_import.c
BOOL K22LoadExtraDlls();
BOOL K22ProcessImports(LPVOID lpImageBase);
VOID K22DisableInitRoutine(LPVOID lpImageBase);
BOOL K22CallInitRoutines(LPVOID lpContext);
BOOL K22DummyEntryPoint(HANDLE hDll, DWORD dwReason, LPVOID lpContext);
// k22_dll_ldrapi.c
BOOL K22LdrApiHookCreate();
BOOL K22LdrApiHookRemove();
// k22_dll_resolve.c
LPCSTR K22ResolveModulePath(LPCSTR lpModuleName, HINSTANCE *ppModule);
HINSTANCE K22ResolveModule(LPCSTR lpCallerName, LPCSTR lpModuleName);
PVOID K22ResolveSymbol(LPCSTR lpCallerName, LPCSTR lpModuleName, LPCSTR lpSymbolName);
#endif
