// Copyright (c) Kuba Szczodrzyński 2024-2-18.

#pragma once

#include "kernel22.h"

BOOL K22PatchProcessVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor);
BOOL K22PatchRemoteImportTable(HANDLE hProcess, LPVOID lpImageBase);
BOOL K22PatchRemotePostInit(HANDLE hProcess, LPVOID lpPebBase, LPSTR lpLibFileName);
