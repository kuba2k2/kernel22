// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "kernel22.h"

BOOL LoaderMain() {
	LPCSTR lpCommandLine	 = K22SkipCommandLinePart(GetCommandLine(), NULL);
	LPCSTR lpApplicationPath = K22GetApplicationPath(lpCommandLine);

	K22_I("Application path: '%s'", lpApplicationPath);
	K22_I("Command line: '%s'", lpCommandLine);

	if (!K22ProcessPatchVersionCheck(11, 0))
		return FALSE;
	K22_I("PE image version check patched");

	PROCESS_INFORMATION stProcessInformation;
	if (!K22CreateProcess(lpApplicationPath, lpCommandLine, &stProcessInformation))
		return FALSE;

	HANDLE hProcess = stProcessInformation.hProcess;
	HANDLE hThread	= stProcessInformation.hThread;
	PEB stPeb;
	if (!K22ProcessReadPeb(hProcess, &stPeb))
		return FALSE;
	LPVOID lpImageBase = stPeb.ImageBaseAddress;
	K22_I(
		"Created process: PID=%d, TID=%d, base=%p",
		stProcessInformation.dwProcessId,
		stProcessInformation.dwThreadId,
		lpImageBase
	);

	if (!K22PatchImportTableProcess(K22_SOURCE_LOADER, hProcess, lpImageBase))
		return FALSE;
	if (!K22ClearBoundImportTableProcess(hProcess, lpImageBase))
		return FALSE;

	K22_I("Process patched successfully");
	if (ResumeThread(hThread) == -1)
		RETURN_K22_F_ERR("Couldn't resume main thread");
	K22_I("Main thread resumed");

#if K22_LOADER_DEBUGGER
	K22_I("Debugging started");
	if (!K22DebugProcess(hProcess, hThread))
		return FALSE;
	K22_I("Debugging finished");
#endif

	DWORD dwReturnCode = WaitForSingleObject(hProcess, 100);
	if (dwReturnCode != WAIT_TIMEOUT)
		K22_W("Process ended with return code %lu", dwReturnCode);
	else
		K22_I("Process detached");

	CloseHandle(hProcess);
	CloseHandle(hThread);

	return TRUE;
}

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		printf("usage: %s <program> [args...]\n", argv[0]);
		return 1;
	}

	return !LoaderMain();
}
