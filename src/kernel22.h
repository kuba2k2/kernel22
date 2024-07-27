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
#include "k22_config.h"
#include "k22_core.h"
#endif

#if K22_VERIFIER
#define K22LogWrite(...)
#endif

// k22_hexdump.c
K22_CORE_PROC VOID K22HexDump(CONST BYTE *pBuf, SIZE_T cbLength, ULONGLONG ullOffset);
K22_CORE_PROC VOID K22HexDumpProcess(HANDLE hProcess, LPCVOID lpAddress, SIZE_T cbLength);
// k22_import_table.c
K22_CORE_PROC BOOL K22ImportTablePatchProcess(BYTE bSource, HANDLE hProcess, LPVOID lpImageBase);
K22_CORE_PROC BOOL K22ImportTablePatchImage(BYTE bSource, LPVOID lpImageBase);
K22_CORE_PROC BOOL K22ImportTablePatchFile(BYTE bSource, HANDLE hFile);
// k22_patch.c
K22_CORE_PROC BOOL K22PatchVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor);
// k22_remote.c
K22_CORE_PROC BOOL K22RemoteAttachToProcess(HANDLE hProcess);
// k22_utils.c
K22_CORE_PROC VOID K22DebugPrintModules();
K22_CORE_PROC BOOL K22DebugDumpModules(LPCSTR lpOutputDir, LPVOID lpImageBase);
K22_CORE_PROC PLDR_DATA_TABLE_ENTRY K22GetLdrEntry(LPVOID lpImageBase);
