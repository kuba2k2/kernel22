// Copyright (c) Kuba Szczodrzyński 2024-2-19.

#include "k22_core.h"

static BOOL K22PostInitLoadLibrary(HANDLE hProcess, LPVOID lpPebBase, LPSTR lpLibFileName);

BOOL K22CoreAttachToProcess(HANDLE hProcess) {
	// find PEB of the debugged process
	PROCESS_BASIC_INFORMATION stProcessBasicInformation;
	if (!NT_SUCCESS(NtQueryInformationProcess(
			hProcess,
			ProcessBasicInformation,
			&stProcessBasicInformation,
			sizeof(stProcessBasicInformation),
			NULL
		)))
		RETURN_K22_F_ERR("Couldn't read process basic information");
	K22_D("Process PEB address: %p", stProcessBasicInformation.PebBaseAddress);
	// read PEB into memory
	PEB stPeb;
	if (!K22ReadProcessMemory(hProcess, stProcessBasicInformation.PebBaseAddress, 0, stPeb))
		RETURN_K22_F_ERR("Couldn't read PEB");
	// extract base memory address
	LPVOID lpImageBase = stPeb.Reserved3[1];
	K22_D("Process image base address: %p", lpImageBase);

	// clear the PE image import table
	K22_I("Terminating import table");
	if (!K22PatchRemoteImportTable(hProcess, lpImageBase))
		return FALSE;

	// patch post-init routine in PEB to load K22 module DLL
	K22_I("Attaching post-init routine");
	if (!K22PostInitLoadLibrary(hProcess, stProcessBasicInformation.PebBaseAddress, "K22Core64.dll"))
		return FALSE;

	return TRUE;
}

static BOOL K22PostInitLoadLibrary(HANDLE hProcess, LPVOID lpPebBase, LPSTR lpLibFileName) {
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
