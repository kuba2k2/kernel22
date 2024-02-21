// Copyright (c) Kuba Szczodrzyński 2024-2-19.

#pragma once

#include "kernel22.h"

/* public Core API */

// k22_attach.c
K22_CORE_PROC BOOL K22CoreAttachToProcess(HANDLE hProcess);
// k22_patch.c
K22_CORE_PROC BOOL K22CorePatchVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor);
K22_CORE_PROC BOOL K22PatchImportTable(PIMAGE_DOS_HEADER pDosHeader, PIMAGE_NT_HEADERS3264 pNt, BYTE bPatcherType);

/* private Core API */

#if K22_CORE
// k22_patch.c
BOOL K22PatchRemoteImportTable(HANDLE hProcess, LPVOID lpImageBase);
BOOL K22PatchLocalImportTable(LPVOID lpImageBase);
#endif
