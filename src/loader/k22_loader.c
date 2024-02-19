// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "k22_loader.h"

LPCSTR K22GetApplicationPath(LPCSTR lpCommandLine) {
	DWORD cbApplicationName = 0;
	K22SkipCommandLinePart(lpCommandLine, &cbApplicationName);

	LPSTR lpApplicationName = LocalAlloc(0, cbApplicationName + 1);
	CopyMemory(lpApplicationName, lpCommandLine, cbApplicationName);
	lpApplicationName[cbApplicationName] = '\0';

	TCHAR szApplicationPath[MAX_PATH] = {0};
	SearchPath(NULL, lpApplicationName, ".exe", sizeof(szApplicationPath), szApplicationPath, NULL);

	LPSTR lpApplicationPath = lpApplicationName;
	if (szApplicationPath[0]) {
		lpApplicationPath = szApplicationPath;
	} else {
		if (lpApplicationPath[cbApplicationName - 1] == '"')
			lpApplicationPath[cbApplicationName - 1] = '\0';
		if (lpApplicationPath[0] == '"')
			lpApplicationPath++;
	}

	LPCSTR lpResult = _strdup(lpApplicationPath);
	LocalFree(lpApplicationName);
	return lpResult;
}

LPCSTR K22SkipCommandLinePart(LPCSTR lpCommandLine, LPDWORD lpPartLength) {
	if (*lpCommandLine == '"') {
		TCHAR c;
		while ((c = *(++lpCommandLine)) && c != '"') {
			if (lpPartLength)
				(*lpPartLength)++;
		}
		// skip closing quotes
		lpCommandLine++;
		if (lpPartLength) {
			(*lpPartLength) += 2; // count opening and closing quotes
		}
	} else {
		TCHAR c;
		while ((c = *(lpCommandLine++)) && c != ' ' && c != '\t') {
			if (lpPartLength)
				(*lpPartLength)++;
		}
	}
	while (*lpCommandLine == ' ' || *lpCommandLine == '\t')
		lpCommandLine++;
	return lpCommandLine;
}

BOOL K22CreateProcess(LPCSTR lpApplicationPath, LPCSTR lpCommandLine, LPPROCESS_INFORMATION lpProcessInformation) {
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

	// create the process in suspended state
	ZeroMemory(lpProcessInformation, sizeof(*lpProcessInformation));
	if (CreateProcess(
			lpApplicationPath,
			(LPSTR)lpCommandLine,
			NULL,
			NULL,
			TRUE,
#if K22_LOADER_DEBUGGER
			CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS | EXTENDED_STARTUPINFO_PRESENT,
#else
			CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
#endif
			NULL,
			NULL,
			&stStartupInfoEx.StartupInfo,
			lpProcessInformation
		) != TRUE) {
		K22_F_ERR("Couldn't create the target process");
		fResult = FALSE;
		goto end;
	}

#if K22_LOADER_DEBUGGER
	// kill the target process when something goes wrong (and the debugger exits prematurely)
	DebugSetProcessKillOnExit(TRUE);
#endif

end:
	DeleteProcThreadAttributeList(stStartupInfoEx.lpAttributeList);
	NtClose(hParentProcess);
	return fResult;
}
