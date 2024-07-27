// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

static VOID K22CoreDllNotification(DWORD dwReason, PLDR_DLL_NOTIFICATION_DATA pData, PVOID pContext);

BOOL K22CoreMain(PIMAGE_K22_HEADER pK22Header) {
	K22_I("Kernel22 Core starting");

	// initialize the global data structure
	if (!K22DataInitialize(pK22Header))
		return FALSE;

	K22_I("Load Source: %c", pK22Header->bSource);
	K22_I("Process Name: %s", pK22Data->lpProcessName);

	// read configuration from registry
	K22_I("Reading configuration");
	if (!K22ConfigRead())
		return FALSE;

	// initialize MinHook
	if (MH_Initialize() != MH_OK)
		RETURN_K22_F("Couldn't initialize MinHook");

	// restore the import directory + first thunk
	K22_I("Restoring import directory");
	if (!K22ImportTableRestore(pK22Data->lpProcessBase))
		return FALSE;

	// import DLL notification functions
	HANDLE hNtdll = GetModuleHandle("ntdll.dll");
	if (hNtdll == INVALID_HANDLE_VALUE)
		RETURN_K22_F_ERR("Couldn't get ntdll.dll handle");
	LdrRegisterDllNotification = (PVOID)GetProcAddress(hNtdll, "LdrRegisterDllNotification");
	if (LdrRegisterDllNotification == NULL)
		RETURN_K22_F_ERR("Couldn't get LdrRegisterDllNotification address");
	LdrUnregisterDllNotification = (PVOID)GetProcAddress(hNtdll, "LdrUnregisterDllNotification");
	if (LdrUnregisterDllNotification == NULL)
		RETURN_K22_F_ERR("Couldn't get LdrUnregisterDllNotification address");

	// register DLL notification callback
	K22_I("Registering DLL notification callback");
	PVOID pCookie;
	if (LdrRegisterDllNotification(0, K22CoreDllNotification, NULL, &pCookie) != ERROR_SUCCESS)
		RETURN_K22_F_ERR("Couldn't register DLL notification");

	// don't call any initialization routines during resolving of static dependencies
	pK22Data->fDelayDllInit = TRUE;
	// process static dependencies of the current process
	K22DebugPrintModules();
	if (!K22ProcessImports(pK22Data->lpProcessBase))
		return FALSE;
	// static dependencies are resolved, call init routines normally from now on
	pK22Data->fDelayDllInit = FALSE;
	// finally call all delayed init routines
	K22DebugPrintModules();
	if (!K22CallInitRoutines())
		return FALSE;

	K22DebugPrintModules();
	K22_I("Kernel22 Core initialized, resuming process");

	return TRUE;
}

static VOID K22CoreDllNotification(DWORD dwReason, PLDR_DLL_NOTIFICATION_DATA pData, PVOID pContext) {
	switch (dwReason) {
		case LDR_DLL_NOTIFICATION_REASON_LOADED:
			PLDR_DATA_TABLE_ENTRY pLdrEntry = K22GetLdrEntry(pData->Loaded.DllBase);
			K22_D(
				"DLL @ %p: %ls - loaded with entry @ %p",
				pData->Loaded.DllBase,
				pData->Loaded.BaseDllName->Buffer,
				pLdrEntry->EntryPoint
			);
			K22ProcessImports(pData->Loaded.DllBase);
			break;
		case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
			K22_D("DLL @ %p: %ls - unloaded", pData->Loaded.DllBase, pData->Loaded.BaseDllName->Buffer);
			break;
		default:
			break;
	}
}
