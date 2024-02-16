// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#pragma once

#include "kernel22.h"

typedef struct {
	HANDLE hProcess;
	HANDLE hThread;
	PROCESS_BASIC_INFORMATION stProcessBasicInformation;
	PEB stPeb;
	PEB_LDR_DATA stPebLdr;
	LPVOID lpBase;
	IMAGE_DOS_HEADER stDosHeader;

	union {
		IMAGE_NT_HEADERS32 stNt32;
		IMAGE_NT_HEADERS64 stNt64;
	};
} DEBUG_INFO, *PDEBUG_INFO, *LPDEBUG_INFO;

BOOL K22DebugProcess(HANDLE hProcess, HANDLE hThread);
