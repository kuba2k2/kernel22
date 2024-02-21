// Copyright (c) Kuba Szczodrzyński 2024-2-12.

#include <stdio.h>
#include <windows.h>
#include <winternl.h>

void printWindowsVersion(const char *method_name, int major, int minor, int build) {
	printf("Windows Version - %-28s = %d.%d.%d\n", method_name, major, minor, build);
}

void pebMethod() {
	PPEB pPeb		   = NtCurrentTeb()->ProcessEnvironmentBlock;
	PDWORD pdwReserved = (PDWORD)&pPeb->Reserved9;
	printWindowsVersion("NtCurrentPeb", pdwReserved[44], pdwReserved[45], pdwReserved[46] & 0xFFFF);
	printf("Windows Version - %-28s = %ls\n", "CSDVersion", ((PUNICODE_STRING)((ULONGLONG)pPeb + 0x2E8))->Buffer);
}

void getVersionExMethod() {
	OSVERSIONINFOEX osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&osvi);
	printWindowsVersion("GetVersionEx", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
}

void getVersionMethod() {
	DWORD version = GetVersion();
	int major	  = LOBYTE(LOWORD(version));
	int minor	  = HIBYTE(LOWORD(version));
	int build	  = 0;
	if (version < 0x80000000)
		build = HIWORD(version);
	printWindowsVersion("GetVersion", major, minor, build);
}

void verifyVersionInfoMethod() {
	OSVERSIONINFOEX osvi;
	DWORDLONG conditionMask = 0;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	// Set the corresponding version to compare against
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 1;
	osvi.dwBuildNumber	= 7601;

	// Prepare condition mask
	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

	// Perform the check
	if (VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, conditionMask)) {
		printWindowsVersion("VerifyVersionInfo", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
	} else {
		printf("VerifyVersionInfo: Not 6.1.7601\n");
	}
}

void rtlGetVersionMethod() {
	HANDLE hMod = LoadLibrary("ntdll.dll");
	typedef void(WINAPI * RtlGetVersion_FUNC)(OSVERSIONINFOEXW *);
	RtlGetVersion_FUNC RtlGetVersion = (RtlGetVersion_FUNC)GetProcAddress(hMod, "RtlGetVersion");
	RTL_OSVERSIONINFOEXW rtlOsVersionInfo;
	RtlGetVersion((RTL_OSVERSIONINFOEXW *)&rtlOsVersionInfo);
	printWindowsVersion(
		"RtlGetVersion",
		rtlOsVersionInfo.dwMajorVersion,
		rtlOsVersionInfo.dwMinorVersion,
		rtlOsVersionInfo.dwBuildNumber
	);
}

void rtlGetNtVersionNumbersMethod() {
	HANDLE hMod = LoadLibrary("ntdll.dll");
	typedef void(WINAPI * RtlGetNtVersionNumbers_FUNC)(DWORD *, DWORD *, DWORD *);
	RtlGetNtVersionNumbers_FUNC RtlGetNtVersionNumbers =
		(RtlGetNtVersionNumbers_FUNC)GetProcAddress(hMod, "RtlGetNtVersionNumbers");
	int major;
	int minor;
	int build;
	RtlGetNtVersionNumbers(&major, &minor, &build);
	printWindowsVersion("RtlGetNtVersionNumbers", major, minor, build & 0xFFFF);
}

void getFileVersionInfoMethod() {
	DWORD dwHandle;
	DWORD dwFileVersionInfoSize = GetFileVersionInfoSize("kernel32.dll", &dwHandle);
	if (dwFileVersionInfoSize != 0) {
		LPVOID lpBuffer = malloc(dwFileVersionInfoSize);
		if (lpBuffer != NULL) {
			if (GetFileVersionInfo("kernel32.dll", dwHandle, dwFileVersionInfoSize, lpBuffer)) {
				VS_FIXEDFILEINFO *pFileInfo;
				UINT uLen;
				if (VerQueryValue(lpBuffer, "\\", (LPVOID *)&pFileInfo, &uLen)) {
					printWindowsVersion(
						"GetFileVersionInfo",
						HIWORD(pFileInfo->dwFileVersionMS),
						LOWORD(pFileInfo->dwFileVersionMS),
						HIWORD(pFileInfo->dwFileVersionLS)
					);
				}
			}
			free(lpBuffer);
		}
	}
}

void readRegistryMethod() {
	HKEY hKey;
	if (RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
			0,
			KEY_READ | KEY_QUERY_VALUE,
			&hKey
		) == ERROR_SUCCESS) {
		char version[255], build[255], buildNumber[255];
		DWORD dwSize = 255;
		if (RegQueryValueEx(hKey, "CurrentVersion", NULL, NULL, (LPBYTE)&version, &dwSize) != ERROR_SUCCESS) {
			return;
		}
		dwSize = 255;
		if (RegQueryValueEx(hKey, "CurrentBuildNumber", NULL, NULL, (LPBYTE)&buildNumber, &dwSize) == ERROR_SUCCESS) {
			printf("Windows Version - %-28s = %s.%s\n", "Registry CurrentBuildNumber", version, buildNumber);
		}
		dwSize = 255;
		if (RegQueryValueEx(hKey, "CurrentBuild", NULL, NULL, (LPBYTE)&build, &dwSize) == ERROR_SUCCESS) {
			printf("Windows Version - %-28s = %s.%s\n", "Registry CurrentBuild", version, build);
		}
		RegCloseKey(hKey);
	}
}

int main(int argc, const char *argv[]) {
	for (int i = 0; i < argc; i++) {
		printf("argv[%d]=%s\n", i, argv[i]);
	}
	pebMethod();
	getVersionExMethod();
	getVersionMethod();
	verifyVersionInfoMethod();
	rtlGetVersionMethod();
	rtlGetNtVersionNumbersMethod();
	getFileVersionInfoMethod();
	readRegistryMethod();
	return 0;
}
