// Copyright (c) Kuba Szczodrzyński 2024-2-25.

#include "kernel22.h"

BOOL K22StringDup(LPCSTR lpInput, DWORD cchInput, LPSTR *ppOutput) {
	if (*ppOutput)
		free(*ppOutput);
	cchInput++; // count the NULL terminator
	K22_MALLOC_LENGTH(*ppOutput, cchInput);
	memcpy(*ppOutput, lpInput, cchInput);
	return TRUE;
}

BOOL K22StringDupFileName(LPCSTR lpInput, DWORD cchInput, LPSTR *ppOutput) {
	if (lpInput[0] != '@')
		return K22StringDup(lpInput, cchInput, ppOutput);
	lpInput++; // skip the @ character; cchInput now accounts for the NULL terminator
	DWORD cchInstallDir = pK22Data->stConfig.cchInstallDir;
	K22_MALLOC_LENGTH(*ppOutput, cchInstallDir + cchInput);
	memcpy(*ppOutput, pK22Data->stConfig.lpInstallDir, cchInstallDir);
	memcpy(*ppOutput + cchInstallDir, lpInput, cchInput);
	return TRUE;
}

BOOL K22StringDupDllTarget(LPCSTR lpInput, DWORD cchInput, LPSTR *ppOutput, LPSTR *ppSymbol) {
	LPSTR lpSymbol = strrchr(lpInput, '!');
	if (lpSymbol) {
		*lpSymbol++ = '\0'; // terminate the string before the symbol name, advance the pointer
		if (!K22StringDup(lpSymbol, strlen(lpSymbol), ppSymbol))
			return FALSE;
		return K22StringDupFileName(lpInput, cchInput, ppOutput);
	}
	return K22StringDupFileName(lpInput, cchInput, ppOutput);
}

BOOL K22PathMatches(LPCSTR lpPath, LPCSTR lpPattern) {
	LPCSTR lpPathName	= strrchr(lpPath, '\\');
	LPCSTR lpTargetName = strrchr(lpPattern, '\\');
	if ((lpPathName && lpTargetName) || (!lpPathName && !lpTargetName))
		// both are absolute or both aren't - they must be identical to match
		return _stricmp(lpPath, lpPattern) == 0;
	if (lpPathName)
		// path is absolute - pattern name is enough to match
		return _stricmp(lpPathName + 1, lpPattern) == 0;
	// pattern is absolute, path isn't - impossible to determine match
	return FALSE;
}

BOOL K22PathIsFile(LPCSTR lpPath) {
	DWORD dwAttrib = GetFileAttributes(lpPath);
	return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL K22PathIsFileEx(LPSTR lpDirectory, DWORD cchDirectory, LPCSTR lpName) {
	LPSTR lpSearchName = lpDirectory + cchDirectory;
	*lpSearchName++	   = '\\';
	*lpSearchName	   = '\0';
	strcpy(lpSearchName, lpName);
	return K22PathIsFile(lpDirectory);
}
