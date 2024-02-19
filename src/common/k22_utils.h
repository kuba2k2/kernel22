// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#pragma once

#include "kernel22.h"

LPCSTR K22GetApplicationPath(LPCSTR lpCommandLine);
LPCSTR K22SkipCommandLinePart(LPCSTR lpCommandLine, LPDWORD lpPartLength);
VOID K22HexDump(CONST BYTE *pBuf, SIZE_T cbLength, ULONGLONG ullOffset);
VOID K22HexDumpProcess(HANDLE hProcess, LPCVOID lpAddress, SIZE_T cbLength);
