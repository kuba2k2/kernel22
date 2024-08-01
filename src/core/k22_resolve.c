// Copyright (c) Kuba Szczodrzyński 2024-7-25.

#include "kernel22.h"

static BOOL K22GetVersionExA(OSVERSIONINFO *osvi) {
	GetVersionExA(osvi);
	osvi->dwMajorVersion = 6;
	osvi->dwMinorVersion = 1;
	osvi->dwBuildNumber	 = 9999;
	return TRUE;
}

static BOOL K22GetVersionExW(OSVERSIONINFOW *osvi) {
	GetVersionExW(osvi);
	osvi->dwMajorVersion = 6;
	osvi->dwMinorVersion = 1;
	osvi->dwBuildNumber	 = 9999;
	return TRUE;
}

PVOID K22ResolveSymbol(LPCSTR lpModuleName, LPCSTR lpSymbolName, WORD wSymbolOrdinal) {
	HANDLE hDll = GetModuleHandle(lpModuleName);
	if (hDll == NULL) {
		hDll = LoadLibrary(lpModuleName);
	}
	if (lpSymbolName) {
		if (strcmp(lpSymbolName, "GetVersionExA") == 0)
			return K22GetVersionExA;
		if (strcmp(lpSymbolName, "GetVersionExW") == 0)
			return K22GetVersionExW;
		return GetProcAddress(hDll, lpSymbolName);
	}
	return GetProcAddress(hDll, (PVOID)(ULONG_PTR)wSymbolOrdinal);
}
