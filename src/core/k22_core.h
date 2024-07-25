// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#pragma once

#include "kernel22.h"

#include "MinHook.h"

// k22_core.c
BOOL K22CoreMain(PIMAGE_K22_HEADER pK22Header);
// k22_data.c
PK22_DATA K22DataGet();
PK22_MODULE_DATA K22DataGetModule(LPVOID lpImageBase);
BOOL K22DataInitialize(LPVOID lpImageBase);
BOOL K22DataInitializeModule(LPVOID lpImageBase);
BOOL K22DataReadConfig();
// k22_file.c
BOOL K22StringDup(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
BOOL K22StringDupFileName(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput);
BOOL K22StringDupDllTarget(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput, LPSTR *ppSymbol);
// k22_import.c
BOOL K22ImportTableRestore(LPVOID lpImageBase);
BOOL K22ProcessImports(LPVOID lpImageBase);
