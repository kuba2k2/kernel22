// Copyright (c) Kuba Szczodrzyński 2024-2-21.

#include "kernel22.h"

#include "rtl_verifier.h"

VOID _RTC_InitBase() {}

VOID _RTC_Shutdown() {}

VOID _RTC_CheckStackVars() {}

VOID __GSHandlerCheck() {}

VOID __report_rangecheckfailure() {}

void __cdecl __security_check_cookie(_In_ uintptr_t _StackCookie) {}

uintptr_t __security_cookie;

static RTL_VERIFIER_DLL_DESCRIPTOR stDll[] = {
	{NULL, 0, NULL, NULL},
};

static RTL_VERIFIER_PROVIDER_DESCRIPTOR stProvider = {
	sizeof(RTL_VERIFIER_PROVIDER_DESCRIPTOR),
	stDll,
	NULL,
	NULL,
	NULL,
	0,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
};

BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, PRTL_VERIFIER_PROVIDER_DESCRIPTOR *ppProvider) {
	// ignore any other events
	if (dwReason != DLL_PROCESS_VERIFIER) {
		return TRUE;
	}
	K22_I("Verifier entry point called with dwReason=%lu, ppProvider=%p", dwReason, ppProvider);
	// set the verifier provider descriptor
	if (ppProvider)
		*ppProvider = &stProvider;

	// patch the current process
	if (!K22PatchImportTable(K22_SOURCE_VERIFIER, NtCurrentPeb()->ImageBaseAddress))
		return FALSE;
	if (!K22ClearBoundImportTable(NtCurrentPeb()->ImageBaseAddress))
		return FALSE;

	return TRUE;
}
