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
	// read NT headers
	IMAGE_NT_HEADERS3264 stNt;
	if (!K22ReadProcessMemory(hProcess, lpImageBase, stDosHeader.e_lfanew, stNt.stNt64))
		RETURN_K22_F_ERR("Couldn't read NT headers");

	// fetch import directory entry
	BOOL fIs64Bit				 = stNt.stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
	IMAGE_DATA_DIRECTORY stEntry = fIs64Bit ? stNt.stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
											: stNt.stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	// go through the import directory
	IMAGE_IMPORT_DESCRIPTOR stDesc;
	for (DWORD dwRvaDesc = stEntry.VirtualAddress; /**/; dwRvaDesc += sizeof(stDesc)) {
		if (!K22ReadProcessMemory(hProcess, lpImageBase, dwRvaDesc, stDesc))
			RETURN_K22_F_ERR("Couldn't read import descriptor @ RVA 0x%x", dwRvaDesc);
		if (stDesc.Characteristics == 0)
			break;

		TCHAR szModuleName[MAX_PATH];
		if (!K22ReadProcessMemoryArray(hProcess, lpImageBase, stDesc.Name, szModuleName))
			RETURN_K22_F_ERR("Couldn't read module name");

		K22_V(
			"Import descriptor @ 0x%x FT=0x%x, OFT=0x%x, Name=0x%x(%s)",
			dwRvaDesc,
			stDesc.FirstThunk,
			stDesc.OriginalFirstThunk,
			stDesc.Name,
			szModuleName
		);

		if (dwRvaDesc == stEntry.VirtualAddress) {
			K22_D("Terminating import table @ RVA 0x%ld", dwRvaDesc);
			stDesc.Characteristics = 0;
			stDesc.FirstThunk	   = 0;
			if (!K22UnlockProcessMemory(hProcess, lpImageBase, dwRvaDesc, sizeof(stDesc), &dwOldProtect))
				RETURN_K22_F_ERR("Couldn't unlock memory @ RVA 0x%x", dwRvaDesc);
			if (!K22WriteProcessMemory(hProcess, lpImageBase, dwRvaDesc, stDesc))
				RETURN_K22_F_ERR("Couldn't write import descriptor @ RVA 0x%x", dwRvaDesc);
		}
	}

	// leave a trace for K22 Core DLL
	CopyMemory(&stDosHeader.e_res, K22_LOADER_COOKIE, 4);
	// write modified DOS header
	if (!K22UnlockProcessMemory(hProcess, lpImageBase, 0, sizeof(stDosHeader), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock DOS header");
	if (!K22WriteProcessMemory(hProcess, lpImageBase, 0, stDosHeader))
		RETURN_K22_F_ERR("Couldn't read DOS header");

	return TRUE;
}
