// Copyright (c) Kuba Szczodrzyński 2024-2-17.

#include "kernel22.h"

VOID LdrDllNotification(ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context) {
	printf("LdrDllNotification(%lu, %p, %p)\n", NotificationReason, NotificationData, Context);
	if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
		printf("FullDllName=%ls\n", NotificationData->Loaded.FullDllName->Buffer);
		printf("BaseDllName=%ls\n", NotificationData->Loaded.BaseDllName->Buffer);
		printf("DllBase=%p\n", NotificationData->Loaded.DllBase);
		printf("SizeOfImage=%lu\n", NotificationData->Loaded.SizeOfImage);

		ULONGLONG ullBase = (ULONGLONG)NotificationData->Loaded.DllBase;

		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)(ullBase + 0);
		PIMAGE_NT_HEADERS3264 pNt	 = (PIMAGE_NT_HEADERS3264)(ullBase + pDosHeader->e_lfanew);

		BOOL fIs64Bit				 = pNt->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
		IMAGE_DATA_DIRECTORY stEntry = fIs64Bit
										   ? pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
										   : pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

		PIMAGE_IMPORT_DESCRIPTOR pDescFirst = (PIMAGE_IMPORT_DESCRIPTOR)(ullBase + stEntry.VirtualAddress);
		K22HexDump((PBYTE)pDescFirst, sizeof(*pDescFirst), (ULONGLONG)pDescFirst);
		for (PIMAGE_IMPORT_DESCRIPTOR pDesc = pDescFirst; /**/; pDesc++) {
			if (pDesc->Characteristics == 0)
				break;

			LPSTR pModuleName = (LPSTR)(ullBase + pDesc->Name);

			K22_D(
				"Import descriptor @ %p FT=0x%x, OFT=0x%x, Name=0x%x(%s)",
				pDesc,
				pDesc->FirstThunk,
				pDesc->OriginalFirstThunk,
				pDesc->Name,
				pModuleName
			);

			if (pDesc == pDescFirst) {
				K22_D("Terminating import table @ %p", pDesc);
				DWORD dwOldProtect;
				if (!VirtualProtect((LPVOID)pDesc, sizeof(*pDesc), PAGE_EXECUTE_READWRITE, &dwOldProtect))
					K22_F_ERR("Couldn't unlock memory @ %p", pDesc);
				pDesc->Characteristics = 0;
				pDesc->FirstThunk	   = 0;
			}
		}
		K22HexDump((PBYTE)pDescFirst, sizeof(*pDescFirst), (ULONGLONG)pDescFirst);
	}
}

BOOL APIENTRY DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	TCHAR szFilename[MAX_PATH + 1];
	GetModuleFileNameA(NULL, szFilename, sizeof(szFilename));

	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			printf("DLL_PROCESS_ATTACH: %s\n", szFilename);
			MessageBox(NULL, szFilename, "DLL_PROCESS_ATTACH", MB_ICONINFORMATION);
			break;
		case DLL_THREAD_ATTACH:
			printf("DLL_THREAD_ATTACH: %s\n", szFilename);
			// MessageBox(NULL, lpFilename, "DLL_THREAD_ATTACH", MB_ICONINFORMATION);
			break;
		case DLL_THREAD_DETACH:
			printf("DLL_THREAD_DETACH: %s\n", szFilename);
			// MessageBox(NULL, lpFilename, "DLL_THREAD_DETACH", MB_ICONINFORMATION);
			break;
		case DLL_PROCESS_DETACH:
			printf("DLL_PROCESS_DETACH: %s\n", szFilename);
			// MessageBox(NULL, lpFilename, "DLL_PROCESS_DETACH", MB_ICONINFORMATION);
			break;
		default:
			break;
	}

	HMODULE hNtdll = GetModuleHandle("ntdll.dll");
	_LdrRegisterDllNotification LdrRegisterDllNotification =
		(_LdrRegisterDllNotification)GetProcAddress(hNtdll, "LdrRegisterDllNotification");

	PVOID pvCookie;
	LdrRegisterDllNotification(0, LdrDllNotification, NULL, &pvCookie);

	return TRUE;
}
