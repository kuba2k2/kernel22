// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#pragma once

#include "kernel22.h"

LPCSTR K22GetApplicationPath(LPCSTR lpCommandLine);
LPCSTR K22SkipCommandLinePart(LPCSTR lpCommandLine, LPDWORD lpPartLength);
BOOL K22CreateDebugProcess(LPCSTR lpApplicationPath, LPCSTR lpCommandLine, LPPROCESS_INFORMATION lpProcessInformation);
