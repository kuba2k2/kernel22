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
	sizeof(RTL_VERIFIER_PROVIDER_DESCRIPTOR), // Length
	stDll,									  // ProviderDlls
	NULL,									  // ProviderDllLoadCallback
	NULL,									  // ProviderDllUnloadCallback
	NULL,									  // VerifierImage
	0,										  // VerifierFlags
	0,										  // VerifierDebug
	NULL,									  // RtlpGetStackTraceAddress
	NULL,									  // RtlpDebugPageHeapCreate
	NULL,									  // RtlpDebugPageHeapDestroy
	NULL,									  // ProviderNtdllHeapFreeCallback
};

#if K22_BITS64
ULONG_PTR GetRBP() {
	// mov rax, rbp
	// ret
	ULONG_PTR pCode = 0x80000000C3E88948;
	PBYTE pFunc		= (PVOID)GetRBP;
	while (*(PULONG_PTR)(++pFunc) != pCode) {}
	return ((ULONG_PTR(*)())pFunc)();
}
#else
#error "Unimplemented platform code"
#endif

BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, PRTL_VERIFIER_PROVIDER_DESCRIPTOR *ppProvider) {
	// ignore any other events
	if (dwReason != DLL_PROCESS_VERIFIER) {
		return TRUE;
	}
	K22_I("Verifier entry point called with dwReason=%lu, ppProvider=%p", dwReason, ppProvider);

	// try disabling Application Verifier
	// fetch AVrfpVerifierProvidersList address from RBP
	PVOID *pProvidersList = (PVOID)GetRBP();
	// set it to its own address, effectively clearing the list
	*pProvidersList = pProvidersList;
	// clear NtGlobalFlag bits
	// ~(FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS | FLG_DISABLE_STACK_EXTENSION)
	NtCurrentPeb()->NtGlobalFlag &= ~(0x100 | 0x02000000 | 0x10000);
	// disable handle verifier
	if (!NT_SUCCESS(NtSetInformationProcess(NtCurrentProcess, ProcessHandleTracing, NULL, 0)))
		RETURN_K22_F_ERR("Couldn't disable ProcessHandleTracing");

	// set the verifier provider descriptor
	if (ppProvider)
		*ppProvider = &stProvider;

	// patch the current process
	if (!K22PatchImportTable(K22_SOURCE_VERIFIER, NtCurrentPeb()->ImageBaseAddress))
		return FALSE;

	return TRUE;
}
