// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "k22_debugger.h"

static BOOL K22DebugEventException(LPDEBUG_INFO lpInfo, LPEXCEPTION_DEBUG_INFO lpEvent);						  // 1
static BOOL K22DebugEventCreateThread(LPDEBUG_INFO lpInfo, DWORD dwThreadId, LPCREATE_THREAD_DEBUG_INFO lpEvent); // 2
static BOOL K22DebugEventCreateProcess(LPDEBUG_INFO lpInfo, LPCREATE_PROCESS_DEBUG_INFO lpEvent);				  // 3
static BOOL K22DebugEventExitThread(LPDEBUG_INFO lpInfo, DWORD dwThreadId, LPEXIT_THREAD_DEBUG_INFO lpEvent);	  // 4
static BOOL K22DebugEventExitProcess(LPDEBUG_INFO lpInfo, LPEXIT_PROCESS_DEBUG_INFO lpEvent);					  // 5
static BOOL K22DebugEventLoadDll(LPDEBUG_INFO lpInfo, LPLOAD_DLL_DEBUG_INFO lpEvent);							  // 6
static BOOL K22DebugEventUnloadDll(LPDEBUG_INFO lpInfo, LPUNLOAD_DLL_DEBUG_INFO lpEvent);						  // 7
static BOOL K22DebugEventOutputString(LPDEBUG_INFO lpInfo, LPOUTPUT_DEBUG_STRING_INFO lpEvent);					  // 8
static BOOL K22DebugEventRipInfo(LPDEBUG_INFO lpInfo, LPRIP_INFO lpEvent);										  // 9

BOOL K22DebugProcess(HANDLE hProcess, HANDLE hThread) {
	DEBUG_INFO stInfo = {
		.hProcess = hProcess,
		.hThread  = hThread,
	};

	// DebugActiveProcessStop(stProcessInformation.dwProcessId);

	BOOL fDebugging = TRUE;
	BOOL fLoaded	= FALSE;

	// receive debug events as long as we need them
	// when something fails - return, while also killing the target process
	while (fDebugging) {
		DEBUG_EVENT stEvent;
		if (!WaitForDebugEvent(&stEvent, INFINITE)) {
			K22_E_ERR("Couldn't receive debug event");
			continue;
		}

		switch (stEvent.dwDebugEventCode) {
			case EXCEPTION_DEBUG_EVENT: /* 1 */
				K22DebugEventException(&stInfo, &stEvent.u.Exception);
				// the first breakpoint indicates finished DLL loading
				if (!fLoaded && stEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT) {
					// ignore any subsequent breakpoints here
					fLoaded = TRUE;
					// don't kill the target if the debugger decides to quit
					DebugSetProcessKillOnExit(FALSE);
				}
				break;
			case CREATE_THREAD_DEBUG_EVENT: /* 2 */
				K22DebugEventCreateThread(&stInfo, stEvent.dwThreadId, &stEvent.u.CreateThread);
				break;
			case CREATE_PROCESS_DEBUG_EVENT: /* 3 */
				K22DebugEventCreateProcess(&stInfo, &stEvent.u.CreateProcessInfo);
				break;
			case EXIT_THREAD_DEBUG_EVENT: /* 4 */
				K22DebugEventExitThread(&stInfo, stEvent.dwThreadId, &stEvent.u.ExitThread);
				break;
			case EXIT_PROCESS_DEBUG_EVENT: /* 5 */
				K22DebugEventExitProcess(&stInfo, &stEvent.u.ExitProcess);
				fDebugging = FALSE;
				break;
			case LOAD_DLL_DEBUG_EVENT: /* 6 */
				K22DebugEventLoadDll(&stInfo, &stEvent.u.LoadDll);
				break;
			case UNLOAD_DLL_DEBUG_EVENT: /* 7 */
				K22DebugEventUnloadDll(&stInfo, &stEvent.u.UnloadDll);
				break;
			case OUTPUT_DEBUG_STRING_EVENT: /* 8 */
				K22DebugEventOutputString(&stInfo, &stEvent.u.DebugString);
				break;
			case RIP_EVENT: /* 9 */
				K22DebugEventRipInfo(&stInfo, &stEvent.u.RipInfo);
				break;
		}
		ContinueDebugEvent(stEvent.dwProcessId, stEvent.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
	}
	return TRUE;
}

static BOOL K22DebugEventException(LPDEBUG_INFO lpInfo, LPEXCEPTION_DEBUG_INFO lpEvent) {
	K22_LOG(
		K22_LEVEL_ERROR,
		__FILE__,
		__LINE__,
		__FUNCTION__,
		RtlNtStatusToDosError(lpEvent->ExceptionRecord.ExceptionCode),
		"Debugger: EXCEPTION_DEBUG_EVENT(Code=0x%lx, Address=%p)",
		lpEvent->ExceptionRecord.ExceptionCode,
		lpEvent->ExceptionRecord.ExceptionAddress
	);
	return TRUE;
}

static BOOL K22DebugEventCreateThread(LPDEBUG_INFO lpInfo, DWORD dwThreadId, LPCREATE_THREAD_DEBUG_INFO lpEvent) {
	K22_D(
		"Debugger: CREATE_THREAD_DEBUG_EVENT(dwThreadId=%d, lpThreadLocalBase=%p, lpStartAddress=%p)",
		dwThreadId,
		lpEvent->lpThreadLocalBase,
		lpEvent->lpStartAddress
	);
	return TRUE;
}

static BOOL K22DebugEventCreateProcess(LPDEBUG_INFO lpInfo, LPCREATE_PROCESS_DEBUG_INFO lpEvent) {
	K22_D(
		"Debugger: CREATE_PROCESS_DEBUG_EVENT(lpBaseOfImage=%p, lpThreadLocalBase=%p, lpStartAddress=%p)",
		lpEvent->lpBaseOfImage,
		lpEvent->lpThreadLocalBase,
		lpEvent->lpStartAddress
	);
	return TRUE;
}

static BOOL K22DebugEventExitThread(LPDEBUG_INFO lpInfo, DWORD dwThreadId, LPEXIT_THREAD_DEBUG_INFO lpEvent) {
	K22_D("Debugger: EXIT_THREAD_DEBUG_EVENT(dwThreadId=%d, dwExitCode=0x%lx)", dwThreadId, lpEvent->dwExitCode);
	return TRUE;
}

static BOOL K22DebugEventExitProcess(LPDEBUG_INFO lpInfo, LPEXIT_PROCESS_DEBUG_INFO lpEvent) {
	K22_D("Debugger: EXIT_PROCESS_DEBUG_EVENT(dwExitCode=0x%lx)", lpEvent->dwExitCode);
	return TRUE;
}

static BOOL K22DebugEventLoadDll(LPDEBUG_INFO lpInfo, LPLOAD_DLL_DEBUG_INFO lpEvent) {
	LPVOID lpImageNamePtr;
	K22ReadProcessMemory(lpInfo->hProcess, lpEvent->lpImageName, 0, lpImageNamePtr);

	WCHAR szImageName[MAX_PATH] = {0};
	if (lpImageNamePtr)
		K22ReadProcessMemoryArray(lpInfo->hProcess, lpImageNamePtr, 0, szImageName);

	if (lpEvent->fUnicode) {
		K22_I("Debugger: LOAD_DLL_DEBUG_EVENT(lpBaseOfDll=%p, szImageName=%ls)", lpEvent->lpBaseOfDll, szImageName);
	} else {
		K22_I("Debugger: LOAD_DLL_DEBUG_EVENT(lpBaseOfDll=%p, szImageName=%s)", lpEvent->lpBaseOfDll, szImageName);
	}
	return TRUE;
}

static BOOL K22DebugEventUnloadDll(LPDEBUG_INFO lpInfo, LPUNLOAD_DLL_DEBUG_INFO lpEvent) {
	K22_I("Debugger: UNLOAD_DLL_DEBUG_EVENT(lpBaseOfDll=%p)", lpEvent->lpBaseOfDll);
	return TRUE;
}

static BOOL K22DebugEventOutputString(LPDEBUG_INFO lpInfo, LPOUTPUT_DEBUG_STRING_INFO lpEvent) {
	WORD wDebugLength	 = lpEvent->nDebugStringLength;
	LPVOID lpDebugString = LocalAlloc(0, wDebugLength);
	K22ReadProcessMemoryLength(lpInfo->hProcess, lpEvent->lpDebugStringData, 0, lpDebugString, wDebugLength);

	// trim trailing newline
	if (lpEvent->fUnicode) {
		PWCHAR pwDebugString = (PWCHAR)lpDebugString;
		if (pwDebugString[wDebugLength / 2 - 2] == '\n')
			pwDebugString[wDebugLength / 2 - 2] = '\0';
	} else {
		PCHAR pcDebugString = (PCHAR)lpDebugString;
		if (pcDebugString[wDebugLength - 2] == '\n')
			pcDebugString[wDebugLength - 2] = '\0';
	}

	// skip duplicated messages
	static WORD wPrevLength	   = 0;
	static LPVOID lpPrevString = NULL;
	if (lpPrevString) {
		// compare with the previous message
		if (wDebugLength == wPrevLength && memcmp(lpDebugString, lpPrevString, wDebugLength) == 0) {
			// free the message since it's identical
			LocalFree(lpDebugString);
			// skip printing it
			return TRUE;
		}
		// message differs - free the previous one
		LocalFree(lpPrevString);
	}
	// keep the new message for next events
	wPrevLength	 = wDebugLength;
	lpPrevString = lpDebugString;

	if (lpEvent->fUnicode) {
		K22_W("Debugger: %ls", lpDebugString);
	} else {
		K22_W("Debugger: %s", lpDebugString);
	}
	return TRUE;
}

static BOOL K22DebugEventRipInfo(LPDEBUG_INFO lpInfo, LPRIP_INFO lpEvent) {
	return TRUE;
}
