// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#pragma once

#include "kernel22.h"

typedef struct {
	HANDLE hProcess;
	HANDLE hThread;
	PROCESS_BASIC_INFORMATION stProcessBasicInformation;
	PEB stPeb;
	LPVOID lpBase;
	DWORD dwModuleLoadThreadId;
} DEBUG_INFO, *PDEBUG_INFO, *LPDEBUG_INFO;

BOOL K22DebugProcess(HANDLE hProcess, HANDLE hThread);
