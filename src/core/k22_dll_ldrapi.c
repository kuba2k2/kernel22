// Copyright (c) Kuba Szczodrzyński 2024-8-6.

#include "kernel22.h"

// LdrLoadDll - used by kernelbase!LoadLibraryExW
// LdrGetDllHandleEx - used by kernelbase!GetModuleHandleForUnicodeString, used by BasepGetModuleHandleExW
// LdrGetProcedureAddress - used by kernelbase!GetProcAddress

K22_HOOK_PROC(
	NTSTATUS,
	LdrLoadDll,
	(PCWSTR pDllPath, PULONG pDllCharacteristics, PUNICODE_STRING pDllName, PVOID *ppDllHandle)
) {
	return RealLdrLoadDll(pDllPath, pDllCharacteristics, pDllName, ppDllHandle);
}

K22_HOOK_PROC(
	NTSTATUS,
	LdrGetDllHandleEx,
	(ULONG ulFlags, PCWSTR pDllPath, PULONG pDllCharacteristics, PUNICODE_STRING pDllName, PVOID *ppDllHandle)
) {
	return RealLdrGetDllHandleEx(ulFlags, pDllPath, pDllCharacteristics, pDllName, ppDllHandle);
}

K22_HOOK_PROC(
	NTSTATUS,
	LdrGetProcedureAddress,
	(PVOID pDllHandle, PANSI_STRING pProcedureName, ULONG ulProcedureNumber, PVOID *ppProcedureAddress)
) {
	return RealLdrGetProcedureAddress(pDllHandle, pProcedureName, ulProcedureNumber, ppProcedureAddress);
}

K22_HOOK_REAL_PROC(
	NTSTATUS,
	LdrLoadDll,
	(PCWSTR pDllPath, PULONG pDllCharacteristics, PUNICODE_STRING pDllName, PVOID *ppDllHandle),
	(pDllPath, pDllCharacteristics, pDllName, ppDllHandle)
);

K22_HOOK_REAL_PROC(
	NTSTATUS,
	LdrGetDllHandleEx,
	(ULONG ulFlags, PCWSTR pDllPath, PULONG pDllCharacteristics, PUNICODE_STRING pDllName, PVOID *ppDllHandle),
	(ulFlags, pDllPath, pDllCharacteristics, pDllName, ppDllHandle)
);

K22_HOOK_REAL_PROC(
	NTSTATUS,
	LdrGetProcedureAddress,
	(PVOID pDllHandle, PANSI_STRING pProcedureName, ULONG ulProcedureNumber, PVOID *ppProcedureAddress),
	(pDllHandle, pProcedureName, ulProcedureNumber, ppProcedureAddress)
);

BOOL K22LdrApiHookCreate() {
	K22_HOOK_CREATE(LdrLoadDll);
	K22_HOOK_CREATE(LdrGetDllHandleEx);
	K22_HOOK_CREATE(LdrGetProcedureAddress);
	return TRUE;
}

BOOL K22LdrApiHookRemove() {
	K22_HOOK_REMOVE(LdrLoadDll);
	K22_HOOK_REMOVE(LdrGetDllHandleEx);
	K22_HOOK_REMOVE(LdrGetProcedureAddress);
	return TRUE;
}
