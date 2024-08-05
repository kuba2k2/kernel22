// Copyright (c) Kuba Szczodrzyński 2024-8-5.

#include "winver.h"

K22_HOOK_PROC(DWORD, GetVersion, ()) {
	PWIN_VER_ENTRY pWinVerEntry = WinVerGetConfig("", GETVERSION);
	if (pWinVerEntry != NULL)
		return (pWinVerEntry->dwMajor << 0) | (pWinVerEntry->dwMinor << 8) | (pWinVerEntry->dwBuild << 16);
	return RealGetVersion();
}

K22_HOOK_PROC(BOOL, GetVersionExA, (LPOSVERSIONINFOA lpVersionInformation)) {
	BOOL bRet = RealGetVersionExA(lpVersionInformation);
	if (bRet) {
		PWIN_VER_ENTRY pWinVerEntry = WinVerGetConfig("", GETVERSIONEXA);
		if (pWinVerEntry != NULL) {
			lpVersionInformation->dwMajorVersion = pWinVerEntry->dwMajor;
			lpVersionInformation->dwMinorVersion = pWinVerEntry->dwMinor;
			lpVersionInformation->dwBuildNumber	 = pWinVerEntry->dwBuild;
		}
	}
	return bRet;
}

K22_HOOK_PROC(BOOL, GetVersionExW, (LPOSVERSIONINFOW lpVersionInformation)) {
	BOOL bRet = RealGetVersionExW(lpVersionInformation);
	if (bRet) {
		PWIN_VER_ENTRY pWinVerEntry = WinVerGetConfig("", GETVERSIONEXW);
		if (pWinVerEntry != NULL) {
			lpVersionInformation->dwMajorVersion = pWinVerEntry->dwMajor;
			lpVersionInformation->dwMinorVersion = pWinVerEntry->dwMinor;
			lpVersionInformation->dwBuildNumber	 = pWinVerEntry->dwBuild;
		}
	}
	return bRet;
}

K22_HOOK_PROC(NTSTATUS, RtlGetVersion, (PRTL_OSVERSIONINFOW lpVersionInformation)) {
	NTSTATUS dwRet = RealRtlGetVersion(lpVersionInformation);
	if (dwRet == ERROR_SUCCESS) {
		PWIN_VER_ENTRY pWinVerEntry = WinVerGetConfig("", RTLGETVERSION);
		if (pWinVerEntry != NULL) {
			lpVersionInformation->dwMajorVersion = pWinVerEntry->dwMajor;
			lpVersionInformation->dwMinorVersion = pWinVerEntry->dwMinor;
			lpVersionInformation->dwBuildNumber	 = pWinVerEntry->dwBuild;
		}
	}
	return dwRet;
}

K22_HOOK_PROC(VOID, RtlGetNtVersionNumbers, (PDWORD pMajorVersion, PDWORD pMinorVersion, PDWORD pBuildNumber)) {
	RealRtlGetNtVersionNumbers(pMajorVersion, pMinorVersion, pBuildNumber);
	PWIN_VER_ENTRY pWinVerEntry = WinVerGetConfig("", RTLGETNTVERSIONNUMBERS);
	if (pWinVerEntry != NULL) {
		*pMajorVersion = pWinVerEntry->dwMajor;
		*pMinorVersion = pWinVerEntry->dwMinor;
		*pBuildNumber  = pWinVerEntry->dwBuild;
	}
}

K22_HOOK_PROC(BOOL, VerQueryValueA, (LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen)) {
	BOOL bRet = RealVerQueryValueA(pBlock, lpSubBlock, lplpBuffer, puLen);
	if (bRet && lpSubBlock && lpSubBlock[0] == '\\') {
		PWIN_VER_ENTRY pWinVerEntry = WinVerGetConfig("", VERQUERYVALUEA);
		if (pWinVerEntry != NULL) {
			VS_FIXEDFILEINFO *pFileInfo = *lplpBuffer;
			pFileInfo->dwFileVersionMS	= (pWinVerEntry->dwMajor << 16) | (pWinVerEntry->dwMinor << 0);
			pFileInfo->dwFileVersionLS	= (pWinVerEntry->dwBuild << 16);
		}
	}
	return bRet;
}

K22_HOOK_PROC(BOOL, VerQueryValueW, (LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen)) {
	BOOL bRet = RealVerQueryValueW(pBlock, lpSubBlock, lplpBuffer, puLen);
	if (bRet && lpSubBlock && lpSubBlock[0] == '\\') {
		PWIN_VER_ENTRY pWinVerEntry = WinVerGetConfig("", VERQUERYVALUEW);
		if (pWinVerEntry != NULL) {
			VS_FIXEDFILEINFO *pFileInfo = *lplpBuffer;
			pFileInfo->dwFileVersionMS	= (pWinVerEntry->dwMajor << 16) | (pWinVerEntry->dwMinor << 0);
			pFileInfo->dwFileVersionLS	= (pWinVerEntry->dwBuild << 16);
		}
	}
	return bRet;
}
