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
	if (!K22DataReadConfig())
		return FALSE;

	// initialize MinHook
	if (MH_Initialize() != MH_OK)
		RETURN_K22_F("Couldn't initialize MinHook");

	// restore the import directory + first thunk
	K22_I("Restoring import directory");
	if (!K22ImportTableRestore(pK22Data->lpProcessBase))
		return FALSE;

	// register DLL notification callback
	K22_I("Registering DLL notification callback");
	HANDLE hNtdll = GetModuleHandle("ntdll.dll");
	if (hNtdll == INVALID_HANDLE_VALUE)
		RETURN_K22_F_ERR("Couldn't get ntdll.dll handle");
	LdrRegisterDllNotification = (PVOID)GetProcAddress(hNtdll, "LdrRegisterDllNotification");
	if (LdrRegisterDllNotification == NULL)
		RETURN_K22_F_ERR("Couldn't get LdrRegisterDllNotification address");
	PVOID pCookie;
	if (LdrRegisterDllNotification(0, K22CoreDllNotification, NULL, &pCookie) != ERROR_SUCCESS)
		RETURN_K22_F_ERR("Couldn't register DLL notification");

	// process imports of the current process
	if (!K22ProcessImports(pK22Data->lpProcessBase))
		return FALSE;

	return TRUE;
}

static VOID K22CoreDllNotification(DWORD dwReason, PLDR_DLL_NOTIFICATION_DATA pData, PVOID pContext) {
	switch (dwReason) {
		case LDR_DLL_NOTIFICATION_REASON_LOADED:
			K22_D("DLL @ %p: %ls - loaded", pData->Loaded.DllBase, pData->Loaded.BaseDllName->Buffer);
			break;
		case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
			K22_D("DLL @ %p: %ls - unloaded", pData->Loaded.DllBase, pData->Loaded.BaseDllName->Buffer);
			break;
		default:
			break;
	}
}
