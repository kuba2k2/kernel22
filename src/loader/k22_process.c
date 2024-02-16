// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "k22_process.h"

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

BOOL K22CreateDebugProcess(LPCSTR lpApplicationPath, LPCSTR lpCommandLine, LPPROCESS_INFORMATION lpProcessInformation) {
	BOOL fResult = TRUE;

	STARTUPINFOEX stStartupInfoEx;
	ZeroMemory(&stStartupInfoEx, sizeof(stStartupInfoEx));
	stStartupInfoEx.StartupInfo.cb = sizeof(stStartupInfoEx.StartupInfo);

	SIZE_T cbProcThreadAttributeList;
	// get structure size
	InitializeProcThreadAttributeList(NULL, 1, 0, &cbProcThreadAttributeList);
	// allocate memory
	stStartupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)LocalAlloc(0, cbProcThreadAttributeList);
	// actually initialize the structure
	InitializeProcThreadAttributeList(stStartupInfoEx.lpAttributeList, 1, 0, &cbProcThreadAttributeList);

	// spoof parent PID of the target process
	// from: VxKex 0.0.0.3
	HANDLE hParentProcess = INVALID_HANDLE_VALUE;
	{
		PROCESS_BASIC_INFORMATION stProcessBasicInformation;
		if (NtQueryInformationProcess(
				GetCurrentProcess(),
				ProcessBasicInformation,
				&stProcessBasicInformation,
				sizeof(stProcessBasicInformation),
				NULL
			) != 0) {
			K22_W_ERR("Couldn't query current process information");
		} else {
			DWORD dwParentId = (DWORD)((ULONGLONG)stProcessBasicInformation.Reserved3);
			hParentProcess	 = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, dwParentId);
			if (hParentProcess == INVALID_HANDLE_VALUE) {
				K22_W_ERR("Couldn't open parent process handle (PPID %ul)", dwParentId);
			} else {
				UpdateProcThreadAttribute(
					stStartupInfoEx.lpAttributeList,
					0,
					PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
					&hParentProcess,
					sizeof(hParentProcess),
					NULL,
					NULL
				);
			}
		}
	}

	K22_D("Application path: '%s'", lpApplicationPath);
	K22_D("Command line: '%s'", lpCommandLine);

	// create the process in suspended state
	ZeroMemory(lpProcessInformation, sizeof(*lpProcessInformation));
	if (CreateProcess(
			lpApplicationPath,
			(LPSTR)lpCommandLine,
			NULL,
			NULL,
			TRUE,
			CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS | EXTENDED_STARTUPINFO_PRESENT,
			NULL,
			NULL,
			&stStartupInfoEx.StartupInfo,
			lpProcessInformation
		) != TRUE) {
		K22_F_ERR("Couldn't create the target process");
		fResult = FALSE;
		goto end;
	}

end:
	DeleteProcThreadAttributeList(stStartupInfoEx.lpAttributeList);
	NtClose(hParentProcess);
	return fResult;
}
