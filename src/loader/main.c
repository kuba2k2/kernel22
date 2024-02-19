// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "kernel22.h"

BOOL LoaderMain() {
	LPCSTR lpCommandLine	 = K22SkipCommandLinePart(GetCommandLine(), NULL);
	LPCSTR lpApplicationPath = K22GetApplicationPath(lpCommandLine);

	K22_I("Application path: '%s'", lpApplicationPath);
	K22_I("Command line: '%s'", lpCommandLine);

	if (!K22CorePatchVersionCheck(11, 0))
		return FALSE;
	K22_I("PE image version check patched");

	PROCESS_INFORMATION stProcessInformation;
	if (!K22CreateProcess(lpApplicationPath, lpCommandLine, &stProcessInformation))
		return FALSE;
	K22_I("Created process: PID=%d, TID=%d", stProcessInformation.dwProcessId, stProcessInformation.dwThreadId);

#if K22_LOADER_DEBUGGER
	if (!K22DebugProcess(stProcessInformation.hProcess, stProcessInformation.hThread))
		return FALSE;
	K22_I("Debugging finished");
#else
	if (!K22CoreAttachToProcess(stProcessInformation.hProcess))
		return FALSE;
	K22_I("Attached K22 Core to process");

	if (ResumeThread(stProcessInformation.hThread) == -1)
		RETURN_K22_F_ERR("Couldn't resume main thread");
	K22_I("Main thread resumed");
#endif

	DWORD dwReturnCode = WaitForSingleObject(stProcessInformation.hProcess, 100);
	if (dwReturnCode != WAIT_TIMEOUT)
		K22_W("Process ended with return code %lu", dwReturnCode);
	else
		K22_I("Process detached");

	CloseHandle(stProcessInformation.hProcess);
	CloseHandle(stProcessInformation.hThread);

	return TRUE;
}

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		printf("usage: %s <program> [args...]\n", argv[0]);
		return 1;
	}

	return !LoaderMain();
}
