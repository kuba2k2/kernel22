// Copyright (c) Kuba Szczodrzyński 2024-2-18.

#include "k22_patch.h"

typedef union {
	IMAGE_NT_HEADERS32 stNt32;
	IMAGE_NT_HEADERS64 stNt64;
} IMAGE_NT_HEADERS3264;

BOOL K22PatchProcessVersionCheck(DWORD dwNewMajor, DWORD dwNewMinor) {
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

		K22_D(
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
			DWORD dwOldProtect;
			if (!K22UnlockProcessMemory(hProcess, lpImageBase, dwRvaDesc, sizeof(stDesc), &dwOldProtect))
				RETURN_K22_F_ERR("Couldn't unlock memory @ RVA 0x%x", dwRvaDesc);
			if (!K22WriteProcessMemory(hProcess, lpImageBase, dwRvaDesc, stDesc))
				RETURN_K22_F_ERR("Couldn't write import descriptor @ RVA 0x%x", dwRvaDesc);
		}
	}
	return TRUE;
}

BOOL K22PatchRemotePostInit(HANDLE hProcess, LPVOID lpPebBase, LPSTR lpLibFileName) {
	BYTE abCode[0x13] =
		// 0000: movabs rax, 0xFFFFFFFFFFBADD11
		"\x48\xb8\x11\xdd\xba\xff\xff\xff\xff\xff"
		// 000A: lea rcx, [rip + 2]
		"\x48\x8d\x0d\x02\x00\x00\x00"
		// 0011: jmp rax
		"\xff\xe0"
		// 0013: module db "module.dll", 0
		// "module.dll\x00";
		"";

	HANDLE hKernel32 = GetModuleHandle("KERNEL32.DLL");
	if (hKernel32 == INVALID_HANDLE_VALUE)
		RETURN_K22_F_ERR("Couldn't get kernel32.dll handle");
	LPVOID lpLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");
	if (lpLoadLibrary == NULL)
		RETURN_K22_F_ERR("Couldn't get LoadLibraryA address");

	K22_D("Remote LoadLibraryA at %p", lpLoadLibrary);
	CopyMemory(abCode + 2, &lpLoadLibrary, 8);

	SIZE_T cbLibFileName = strlen(lpLibFileName) + 1;
	LPVOID lpCodeRemote	 = VirtualAllocEx(
		 hProcess,
		 NULL,
		 // add module filename to the code
		 sizeof(abCode) + strlen(lpLibFileName),
		 MEM_COMMIT,
		 PAGE_EXECUTE_READWRITE
	 );
	if (lpCodeRemote == NULL)
		RETURN_K22_F_ERR("Couldn't allocate remote code memory");
	if (!K22WriteProcessMemoryArray(hProcess, lpCodeRemote, 0, abCode))
		RETURN_K22_F_ERR("Couldn't write remote code");
	if (!K22WriteProcessMemoryLength(hProcess, lpCodeRemote, sizeof(abCode), lpLibFileName, cbLibFileName))
		RETURN_K22_F_ERR("Couldn't write remote library name");
	K22_D("Remote post-init routine at %p", lpCodeRemote);

	K22HexDumpProcess(hProcess, lpCodeRemote, 64);

	// read current PEB
	PEB stPeb;
	if (!K22ReadProcessMemory(hProcess, lpPebBase, 0, stPeb))
		RETURN_K22_F_ERR("Couldn't read PEB");
	// set post-init routine in PEB
	stPeb.PostProcessInitRoutine = lpCodeRemote;
	// write patched PEB
	DWORD dwOldProtect;
	if (!K22UnlockProcessMemory(hProcess, lpPebBase, 0, sizeof(stPeb), &dwOldProtect))
		RETURN_K22_F_ERR("Couldn't unlock PEB");
	if (!K22WriteProcessMemory(hProcess, lpPebBase, 0, stPeb))
		RETURN_K22_F_ERR("Couldn't write PEB");
	return TRUE;
}
