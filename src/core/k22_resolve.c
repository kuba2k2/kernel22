// Copyright (c) Kuba Szczodrzyński 2024-7-25.

#include "kernel22.h"

static BOOL K22GetVersionExA(OSVERSIONINFO *osvi) {
	osvi->dwMajorVersion = 10;
	osvi->dwMinorVersion = 0;
	osvi->dwBuildNumber	 = 10240;
	return TRUE;
}

PVOID K22ResolveSymbol(LPCSTR lpDllName, LPCSTR lpSymbolName, ULONG_PTR ulSymbolOrdinal) {
	HANDLE hDll = GetModuleHandle(lpDllName);
	if (hDll == NULL) {
		hDll = LoadLibrary(lpDllName);
	}
	if (lpSymbolName) {
		if (strcmp(lpSymbolName, "GetVersionExA") == 0)
			return K22GetVersionExA;
		return GetProcAddress(hDll, lpSymbolName);
	}
	return GetProcAddress(hDll, (PVOID)ulSymbolOrdinal);
}
