// Copyright (c) Kuba Szczodrzyński 2024-8-4.

#include "winver.h"

BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpContext) {
	if (dwReason != DLL_PROCESS_ATTACH)
		return TRUE;
	// read configuration
	K22ConfigReadKey("WinVer", WinVerParseConfig);
	// enable all modes if entries are set
	if (fWinVerModes == 0 && pWinVerEntries)
		fWinVerModes = 0xFFFFFFFF;
	// create function hooks
	if (fWinVerModes & GETVERSION)
		K22_HOOK_CREATE(GetVersion);
	if (fWinVerModes & GETVERSIONEXA)
		K22_HOOK_CREATE(GetVersionExA);
	if (fWinVerModes & GETVERSIONEXW)
		K22_HOOK_CREATE(GetVersionExW);
	if (fWinVerModes & RTLGETVERSION)
		K22_HOOK_CREATE(RtlGetVersion);
	if (fWinVerModes & RTLGETNTVERSIONNUMBERS)
		K22_HOOK_CREATE(RtlGetNtVersionNumbers);
	if (fWinVerModes & VERQUERYVALUEA) {
		HANDLE hModule = GetModuleHandle("version.dll");
		if (hModule)
			K22_HOOK_CREATE_EX(VerQueryValueA, GetProcAddress(hModule, "VerQueryValueA"));
	}
	if (fWinVerModes & VERQUERYVALUEW) {
		HANDLE hModule = GetModuleHandle("version.dll");
		if (hModule)
			K22_HOOK_CREATE_EX(VerQueryValueW, GetProcAddress(hModule, "VerQueryValueW"));
	}
	return TRUE;
}
