// Copyright (c) Kuba Szczodrzyński 2024-2-18.

#include "k22_core.h"

BOOL K22CorePatchVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor) {
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

BOOL K22PatchRemoteImportTable(HANDLE hProcess, LPVOID lpImageBase) {
	DWORD dwOldProtect;

	// read DOS header
	IMAGE_DOS_HEADER stDosHeader;
	if (!K22ReadProcessMemory(hProcess, lpImageBase, 0, stDosHeader))
		RETURN_K22_F_ERR("Couldn't read DOS header");
	// read NT header
	IMAGE_NT_HEADERS3264 stNt;
	if (!K22ReadProcessMemory(hProcess, lpImageBase, stDosHeader.e_lfanew, stNt))
		RETURN_K22_F_ERR("Couldn't read NT header");

	// get a handle to K22 data in DOS header
	PK22_HDR_DATA pK22HdrData = K22_DOS_HDR_DATA(&stDosHeader);
	// fetch import directory entry
	BOOL fIs64Bit				 = stNt.stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
	PIMAGE_DATA_DIRECTORY pEntry = fIs64Bit ? &stNt.stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
											: &stNt.stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	// disable the import directory
	K22_D("Import Directory @ RVA %p", pEntry->VirtualAddress);
	pK22HdrData->dwRvaImportDirectory = pEntry->VirtualAddress;
	pEntry->VirtualAddress			  = 0;

	// leave a trace for K22 Core DLL
	CopyMemory(pK22HdrData->abPatcherCookie, K22_PATCHER_COOKIE, 3);
	pK22HdrData->bPatcherType = K22_PATCHER_MEMORY;

	// write modified DOS header
	if (!K22UnlockProcessMemory(hProcess, lpImageBase, 0, sizeof(stDosHeader), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock DOS header");
	if (!K22WriteProcessMemory(hProcess, lpImageBase, 0, stDosHeader))
		RETURN_K22_F_ERR("Couldn't write DOS header");
	// write modified NT header
	if (!K22UnlockProcessMemory(hProcess, lpImageBase, stDosHeader.e_lfanew, sizeof(stNt), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock NT header");
	if (!K22WriteProcessMemory(hProcess, lpImageBase, stDosHeader.e_lfanew, stNt))
		RETURN_K22_F_ERR("Couldn't write NT header");

	return TRUE;
}
