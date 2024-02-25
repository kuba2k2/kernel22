// Copyright (c) Kuba Szczodrzyński 2024-2-25.

#pragma once

#include "kernel22.h"

BOOL K22DataDupString(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput) {
	if (*ppOutput)
		free(*ppOutput);
	cchInput++; // count the NULL terminator
	K22_MALLOC_LENGTH(*ppOutput, cchInput);
	memcpy(*ppOutput, lpInput, cchInput);
	return TRUE;
}

BOOL K22DataDupFileName(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput) {
	if (lpInput[0] != '@')
		return K22DataDupString(lpInput, cchInput, ppOutput);
	lpInput++; // skip the @ character; cchInput now accounts for the NULL terminator
	DWORD cchInstallDir = pK22Data->stConfig.cchInstallDir;
	K22_MALLOC_LENGTH(*ppOutput, cchInstallDir + cchInput);
	memcpy(*ppOutput, pK22Data->stConfig.lpInstallDir, cchInstallDir);
	memcpy(*ppOutput + cchInstallDir, lpInput, cchInput);
	return TRUE;
}

BOOL K22DataDupDllTarget(LPSTR lpInput, DWORD cchInput, LPSTR *ppOutput, LPSTR *ppSymbol) {
	LPSTR lpSymbol = strrchr(lpInput, '!');
	if (lpSymbol) {
		*lpSymbol++ = '\0'; // terminate the string before the symbol name, advance the pointer
		if (!K22DataDupString(lpSymbol, strlen(lpSymbol), ppSymbol))
			return FALSE;
		return K22DataDupFileName(lpInput, cchInput, ppOutput);
	}
	return K22DataDupFileName(lpInput, cchInput, ppOutput);
}
