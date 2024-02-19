// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#include "k22_utils.h"

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

VOID K22HexDump(CONST BYTE *pBuf, SIZE_T cbLength, ULONGLONG ullOffset) {
	DWORD cbPos = 0;
	while (cbPos < cbLength) {
		// print hex offset
		printf("%08llx ", ullOffset + cbPos);
		// calculate current line width
		BYTE bLineWidth = (cbLength - cbPos) < 16 ? (cbLength - cbPos) : 16;
		// print hexadecimal representation
		for (DWORD i = 0; i < bLineWidth; i++) {
			if (i % 8 == 0) {
				printf(" ");
			}
			printf("%02x ", pBuf[cbPos + i]);
		}
		// print ascii representation
		printf(" |");
		for (DWORD i = 0; i < bLineWidth; i++) {
			BYTE c = pBuf[cbPos + i];
			putchar((c >= 0x20 && c <= 0x7f) ? c : '.');
		}
		puts("|\r");
		cbPos += bLineWidth;
	}
}

VOID K22HexDumpProcess(HANDLE hProcess, LPCVOID lpAddress, SIZE_T cbLength) {
	PBYTE bMemory = LocalAlloc(0, cbLength);
	SIZE_T cbMemory;
	ReadProcessMemory(hProcess, lpAddress, bMemory, cbLength, &cbMemory);
	K22HexDump(bMemory, cbLength, (ULONGLONG)lpAddress);
	LocalFree(bMemory);
}
