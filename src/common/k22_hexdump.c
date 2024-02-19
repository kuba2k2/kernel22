// Copyright (c) Kuba Szczodrzyński 2024-2-19.

#include "k22_hexdump.h"

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
