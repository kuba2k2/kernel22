// Copyright (c) Kuba Szczodrzyński 2024-2-17.

#include "kernel22.h"

// dummy function used to import Core DLL
K22_CORE_PROC VOID DllLd() {}

static BOOL DllInitialize(HANDLE hDll, LPVOID lpContext) {
	LPVOID lpProcessBase = GetModuleHandle(NULL);

	// ignore processes not patched by K22 Core
	PIMAGE_K22_HEADER pK22Header = K22_DOS_HDR_DATA(lpProcessBase);
	if (memcmp(pK22Header->bCookie, K22_COOKIE, 3) != 0)
		return TRUE;

	return K22CoreMain(pK22Header, lpContext);
}

static VOID DllError() {
	K22LogShowErrorMessage("Kernel22 Core initialization failed:\r\n\r\n");
}

#pragma warning(push)
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "ConstantFunctionResult"
#pragma ide diagnostic ignored "ConstantConditionsOC"

BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpContext) {
	// ignore any other events
	if (dwReason != DLL_PROCESS_ATTACH)
		return TRUE;
	if (!DllInitialize(hDll, lpContext)) {
		DllError();
		return FALSE;
	}
	return TRUE;
}

#pragma clang diagnostic pop
#pragma warning(pop)
