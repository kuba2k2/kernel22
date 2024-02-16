// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "kernel22.h"

BOOL LoaderMain() {
	LPCSTR lpCommandLine	 = K22SkipCommandLinePart(GetCommandLine(), NULL);
	LPCSTR lpApplicationPath = K22GetApplicationPath(lpCommandLine);

	if (!K22PatchProcessVersionCheck(11, 0))
		return FALSE;

	PROCESS_INFORMATION stProcessInformation;
	if (!K22CreateDebugProcess(lpApplicationPath, lpCommandLine, &stProcessInformation))
		return FALSE;

	K22_I("Process created: PID=%d, TID=%d", stProcessInformation.dwProcessId, stProcessInformation.dwThreadId);

	if (!K22DebugProcess(stProcessInformation.hProcess, stProcessInformation.hThread))
		return FALSE;

	DWORD dwReturnCode = WaitForSingleObject(stProcessInformation.hProcess, 1000);
	K22_I("Process ended with return code %lu", dwReturnCode);

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
