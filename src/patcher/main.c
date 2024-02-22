// Copyright (c) Kuba Szczodrzyński 2024-2-20.

#include "kernel22.h"

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		printf("usage: %s <filename>\n", argv[0]);
		return 1;
	}

	LPCSTR lpImageName = argv[1];
	HANDLE hFile =
		CreateFile(lpImageName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		K22_F_ERR("Couldn't open the file '%s'", lpImageName);
		return 2;
	}

	if (!K22ImportTablePatchFile(K22_SOURCE_PATCHER, hFile)) {
		CloseHandle(hFile);
		return 3;
	}

	K22_I("File '%s' patched successfully", lpImageName);
	CloseHandle(hFile);

	return 0;
}
