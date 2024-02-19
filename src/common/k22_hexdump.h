// Copyright (c) Kuba Szczodrzyński 2024-2-19.

#pragma once

#include "kernel22.h"

VOID K22HexDump(CONST BYTE *pBuf, SIZE_T cbLength, ULONGLONG ullOffset);
VOID K22HexDumpProcess(HANDLE hProcess, LPCVOID lpAddress, SIZE_T cbLength);
