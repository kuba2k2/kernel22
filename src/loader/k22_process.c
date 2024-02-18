// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "k22_process.h"

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
