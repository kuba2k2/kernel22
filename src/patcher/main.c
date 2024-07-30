// Copyright (c) Kuba Szczodrzyński 2024-2-20.

#include "kernel22.h"

BOOL PatcherMain(LPCSTR lpImageName, BOOL fPatch) {
	HANDLE hFile =
		CreateFile(lpImageName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		K22_F_ERR("Couldn't open the file '%s'", lpImageName);
		return FALSE;
	}

	if (!K22PatchImportTableFile(fPatch ? K22_SOURCE_PATCHER : K22_SOURCE_NONE, hFile))
		goto error;

	K22_I("File '%s' %s successfully", lpImageName, fPatch ? "patched" : "unpatched");

	CloseHandle(hFile);
	return TRUE;
error:
	CloseHandle(hFile);
	return FALSE;
}

BOOL PatcherHelp(LPCSTR lpProgramName) {
	printf(
		"Patches an .EXE file to inject K22 Core DLL on startup.\n"
		"\n"
		"%s [/U] filename\n"
		"\n"
		"    filename    Specifies the .EXE file to patch.\n"
		"    /U          Allows to unpatch a previously patched file.\n"
		"    /?          Shows this help message.\n",
		lpProgramName
	);
	return TRUE;
}

int main(int argc, const char *argv[]) {
	LPCSTR lpImageName = NULL;
	BOOL fPatch		   = TRUE;

	for (int i = 1; i < argc; i++) {
		if (_stricmp(argv[i], "/U") == 0)
			fPatch = FALSE;
		else if (argv[i][0] == '/' || lpImageName)
			return !PatcherHelp(argv[0]);
		else
			lpImageName = argv[i];
	}

	if (!lpImageName)
		return !PatcherHelp(argv[0]);

	return !PatcherMain(lpImageName, fPatch);
}
