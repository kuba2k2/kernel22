// Copyright (c) Kuba Szczodrzyński 2024-2-17.

#include "k22_core.h"

static BOOL DllInitialize(DWORD dwReason) {
	// ignore any other events
	if (dwReason != DLL_PROCESS_ATTACH)
		return TRUE;
	// ignore processes not started by the loader (or by a patched process)
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);
	if (memcmp(&pDosHeader->e_res, K22_LOADER_COOKIE, 4) != 0)
		return TRUE;

	TCHAR szFilename[MAX_PATH + 1];
	GetModuleFileNameA(NULL, szFilename, sizeof(szFilename));
	printf("DLL_PROCESS_ATTACH: %s\n", szFilename);
	MessageBox(NULL, szFilename, "DLL_PROCESS_ATTACH", MB_ICONINFORMATION);

	return TRUE;
}

static VOID DllError() {
	MessageBox(0, "Couldn't initialize K22 Core DLL", "Error", MB_ICONERROR);
}

#pragma warning(push)
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "ConstantFunctionResult"
#pragma ide diagnostic ignored "ConstantConditionsOC"

BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved) {
	if (!DllInitialize(dwReason)) {
		DllError();
		return FALSE;
	}
	return TRUE;
}

#pragma clang diagnostic pop
#pragma warning(pop)
