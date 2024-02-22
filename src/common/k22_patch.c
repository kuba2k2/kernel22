// Copyright (c) Kuba Szczodrzyński 2024-2-18.

#include "kernel22.h"

BOOL K22PatchVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor) {
	HANDLE hKernel32	   = GetModuleHandle("kernel32.dll");
	HANDLE hCurrentProcess = GetCurrentProcess();

	static LPDWORD lpdwFakeVersion = NULL;
	if (lpdwFakeVersion == NULL) {
		// allocate 2 DWORD values in memory
		lpdwFakeVersion = VirtualAlloc(NULL, 2 * sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (lpdwFakeVersion == NULL)
			RETURN_K22_F_ERR("Couldn't allocate memory");
		VirtualProtect(lpdwFakeVersion, 2 * sizeof(DWORD), PAGE_READONLY, NULL);
	}

	// set new fake version numbers
	lpdwFakeVersion[0] = dwNewMajor;
	lpdwFakeVersion[1] = dwNewMinor;

	// get address of kernel32 in the loader process
	MODULEINFO stModuleInfo;
	ZeroMemory(&stModuleInfo, sizeof(stModuleInfo));
	if (!GetModuleInformation(hCurrentProcess, hKernel32, &stModuleInfo, sizeof(MODULEINFO)))
		RETURN_K22_E_ERR("Couldn't retrieve kernel32.dll module information (handle=%p)", hKernel32);

	// search the kernel32 image for KUSER_SHARED_DATA pointers
	BOOL fMajorFound = FALSE;
	BOOL fMinorFound = FALSE;
	LPBYTE lpbEnd	 = (LPBYTE)stModuleInfo.lpBaseOfDll + stModuleInfo.SizeOfImage;
	for (LPBYTE lpbData = stModuleInfo.lpBaseOfDll; lpbData < lpbEnd - sizeof(DWORD); lpbData++) {
		DWORD dwOldProtect;
		LPDWORD lpdwData = (LPDWORD)lpbData;

		if (*lpdwData == 0x7FFE026C) { // KUSER_SHARED_DATA.NtMajorVersion
			// replace the pointer
			VirtualProtect(lpdwData, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
			*lpdwData = (DWORD)(ULONGLONG)&lpdwFakeVersion[0];
			VirtualProtect(lpdwData, sizeof(DWORD), dwOldProtect, NULL);

			fMajorFound = TRUE;
			if (fMinorFound)
				break;
			continue;
		}

		if (*lpdwData == 0x7FFE0270) { // KUSER_SHARED_DATA.NtMinorVersion
			// replace the pointer
			VirtualProtect(lpdwData, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
			*lpdwData = (DWORD)(ULONGLONG)&lpdwFakeVersion[1];
			VirtualProtect(lpdwData, sizeof(DWORD), dwOldProtect, NULL);

			fMinorFound = TRUE;
			if (fMajorFound)
				break;
			continue;
		}
	}

	if (!(fMajorFound && fMinorFound))
		RETURN_K22_E("Couldn't find KUSER_SHARED_DATA refs in kernel32: %d/%d", fMajorFound, fMinorFound);
	return TRUE;
}
