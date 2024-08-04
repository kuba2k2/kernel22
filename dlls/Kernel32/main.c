// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpContext) {
	if (dwReason != DLL_PROCESS_ATTACH)
		return TRUE;
	return TRUE;
}
