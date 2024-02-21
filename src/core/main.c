// Copyright (c) Kuba Szczodrzyński 2024-2-17.

#include "kernel22.h"

static BOOL DllInitialize(HANDLE hDll) {
	LPVOID lpProcessBase = GetModuleHandle(NULL);

	// ignore processes not patched by K22 Core
	PK22_HDR_DATA pK22HdrData = K22_DOS_HDR_DATA(lpProcessBase);
	if (memcmp(pK22HdrData->abPatcherCookie, K22_PATCHER_COOKIE, 3) != 0)
		return TRUE;

	K22_D("Import Directory @ RVA %p", pK22HdrData->dwRvaImportDirectory);

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
	// ignore any other events
	if (dwReason != DLL_PROCESS_ATTACH)
		return TRUE;
	if (!DllInitialize(hDll)) {
		DllError();
		return FALSE;
	}
	return TRUE;
}

#pragma clang diagnostic pop
#pragma warning(pop)
