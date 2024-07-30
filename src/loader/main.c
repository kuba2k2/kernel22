// Copyright (c) Kuba Szczodrzyński 2024-2-16.

#include "kernel22.h"

BOOL LoaderMain(DWORD dwCommandLineSkip, DWORD dwDebug) {
	LPCSTR lpCommandLine = GetCommandLine();
	while (dwCommandLineSkip--) {
		lpCommandLine = K22SkipCommandLinePart(lpCommandLine, NULL);
	}
	LPCSTR lpApplicationPath = K22GetApplicationPath(lpCommandLine);

	K22_I("Application path: '%s'", lpApplicationPath);
	K22_I("Command line: '%s'", lpCommandLine);

	if (dwDebug == -1) {
		HKEY hMain;
		K22_REG_VARS();
		K22_REG_REQUIRE_KEY(HKEY_LOCAL_MACHINE, K22_REG_KEY_PATH, hMain);
		K22_REG_READ_VALUE(hMain, "EnableDebuggerInLoader", &dwDebug, cbValue);
		RegCloseKey(hMain);
	}
	if (dwDebug == -1)
		dwDebug = 0;

	if (!K22ProcessPatchVersionCheck(11, 0))
		return FALSE;
	K22_I("PE image version check patched");

	PROCESS_INFORMATION stProcessInformation;
	if (!K22CreateProcess(lpApplicationPath, lpCommandLine, &stProcessInformation, (BOOL)dwDebug))
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
	if (!K22PatchBoundImportTableProcess(TRUE, hProcess, lpImageBase))
		return FALSE;

	K22_I("Process patched successfully");
	if (ResumeThread(hThread) == -1)
		RETURN_K22_F_ERR("Couldn't resume main thread");
	K22_I("Main thread resumed");

	if ((BOOL)dwDebug) {
		K22_I("Debugging started");
		if (!K22DebugProcess(hProcess, hThread))
			return FALSE;
		K22_I("Debugging finished");
	}

	DWORD dwReturnCode = WaitForSingleObject(hProcess, 100);
	if (dwReturnCode != WAIT_TIMEOUT)
		K22_W("Process ended with return code %lu", dwReturnCode);
	else
		K22_I("Process detached");

	CloseHandle(hProcess);
	CloseHandle(hThread);

	return TRUE;
}

BOOL LoaderHelp(LPCSTR lpProgramName) {
	printf(
		"Runs an .EXE program with K22 Core DLL.\n"
		"\n"
		"%s [/D | /-D] program [arguments]\n"
		"\n"
		"    filename    Specifies the program to run.\n"
		"    arguments   Allows to pass command line arguments.\n"
		"    /D          Enables the built-in program debugger.\n"
		"    /-D         Disables the built-in program debugger.\n"
		"    /?          Shows this help message.\n"
		"\n"
		"The switch /D or /-D allows to enable/disable the debugger,\n"
		"regardless of the global registry setting.\n"
		"\n"
		"All switches must precede the program name and arguments.\n",
		lpProgramName
	);
	return TRUE;
}

int main(int argc, const char *argv[]) {
	DWORD dwDebug = -1;

	for (int i = 1; i < argc; i++) {
		if (_stricmp(argv[i], "/D") == 0)
			dwDebug = 1;
		else if (_stricmp(argv[i], "/-D") == 0)
			dwDebug = 0;
		else if (argv[i][0] == '/')
			return !LoaderHelp(argv[0]);
		else
			// finish parsing on first non-switch argument
			return !LoaderMain(i, dwDebug);
	}

	// program name not found
	return !LoaderHelp(argv[0]);
}
