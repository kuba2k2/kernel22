// Copyright (c) Kuba Szczodrzyński 2024-2-17.

#include "kernel22.h"

BOOL APIENTRY DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	TCHAR szFilename[MAX_PATH + 1];
	GetModuleFileNameA(NULL, szFilename, sizeof(szFilename));

	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			printf("DLL_PROCESS_ATTACH: %s\n", szFilename);
			MessageBox(NULL, szFilename, "DLL_PROCESS_ATTACH", MB_ICONINFORMATION);
			break;
		case DLL_THREAD_ATTACH:
			printf("DLL_THREAD_ATTACH: %s\n", szFilename);
			// MessageBox(NULL, lpFilename, "DLL_THREAD_ATTACH", MB_ICONINFORMATION);
			break;
		case DLL_THREAD_DETACH:
			printf("DLL_THREAD_DETACH: %s\n", szFilename);
			// MessageBox(NULL, lpFilename, "DLL_THREAD_DETACH", MB_ICONINFORMATION);
			break;
		case DLL_PROCESS_DETACH:
			printf("DLL_PROCESS_DETACH: %s\n", szFilename);
			// MessageBox(NULL, lpFilename, "DLL_PROCESS_DETACH", MB_ICONINFORMATION);
			break;
		default:
			break;
	}
	return TRUE;
}
