// Copyright (c) Kuba Szczodrzyński 2024-2-19.

#pragma once

#include "kernel22.h"

/* public Core API */

// core_attach.c
K22_CORE_PROC BOOL K22CoreAttachToProcess(HANDLE hProcess);
// core_patch.c
K22_CORE_PROC BOOL K22CorePatchVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor);

/* private Core API */

#if K22_CORE
// core_patch.c
BOOL K22PatchRemoteImportTable(HANDLE hProcess, LPVOID lpImageBase);
#endif
