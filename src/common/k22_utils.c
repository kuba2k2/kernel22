// Copyright (c) Kuba Szczodrzyński 2024-2-25.

#include "kernel22.h"

VOID K22DebugPrintModules() {
	K22_LDR_ENUM(pLdrEntry) {
		LPSTR lpReason = NULL;
		switch (pLdrEntry->LoadReason) {
			case LoadReasonStaticDependency:
				lpReason = "LoadReasonStaticDependency";
				break;
			case LoadReasonStaticForwarderDependency:
				lpReason = "LoadReasonStaticForwarderDependency";
				break;
			case LoadReasonDynamicForwarderDependency:
				lpReason = "LoadReasonDynamicForwarderDependency";
				break;
			case LoadReasonDelayloadDependency:
				lpReason = "LoadReasonDelayloadDependency";
				break;
			case LoadReasonDynamicLoad:
				lpReason = "LoadReasonDynamicLoad";
				break;
			case LoadReasonAsImageLoad:
				lpReason = "LoadReasonAsImageLoad";
				break;
			case LoadReasonAsDataLoad:
				lpReason = "LoadReasonAsDataLoad";
				break;
			case LoadReasonEnclavePrimary:
				lpReason = "LoadReasonEnclavePrimary";
				break;
			case LoadReasonEnclaveDependency:
				lpReason = "LoadReasonEnclaveDependency";
				break;
			case LoadReasonUnknown:
				lpReason = "LoadReasonUnknown";
				break;
			default:
				lpReason = "???";
				break;
		}
		K22_D(
			"DLL @ %p: %ls (size %lu) - %s (%d)",
			pLdrEntry->DllBase,
			pLdrEntry->BaseDllName.Buffer,
			pLdrEntry->SizeOfImage,
			lpReason,
			pLdrEntry->LoadReason
		);
	}
}

BOOL K22DebugDumpModules(LPCSTR lpOutputDirName, LPVOID lpImageBase) {
	CreateDirectory(lpOutputDirName, NULL);
	K22_LDR_ENUM(pLdrEntry) {
		if (lpImageBase && lpImageBase != pLdrEntry->DllBase)
			continue;
		ANSI_STRING szModuleName;
		RtlUnicodeStringToAnsiString(&szModuleName, &pLdrEntry->BaseDllName, TRUE);

		DWORD cchOutputDirName = strlen(lpOutputDirName);
		LPSTR lpOutputFile;
		K22_MALLOC_LENGTH(lpOutputFile, cchOutputDirName + 1 + strlen(szModuleName.Buffer) + 1);

		memcpy(lpOutputFile, lpOutputDirName, cchOutputDirName);
		lpOutputFile[cchOutputDirName] = '\\';
		memcpy(lpOutputFile + cchOutputDirName + 1, szModuleName.Buffer, szModuleName.Length + 1);
		RtlFreeAnsiString(&szModuleName);

		K22_I("Dumping %lu bytes to %s", pLdrEntry->SizeOfImage, lpOutputFile);
		HANDLE hFile = CreateFile(lpOutputFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			free(lpOutputFile);
			RETURN_K22_F_ERR("Couldn't open file for writing");
		}
		DWORD dwBytesWritten = 0;
		WriteFile(hFile, pLdrEntry->DllBase, pLdrEntry->SizeOfImage, &dwBytesWritten, NULL);
		CloseHandle(hFile);
		free(lpOutputFile);
	}
	return TRUE;
}

PLDR_DATA_TABLE_ENTRY K22GetLdrEntry(LPVOID lpImageBase) {
	K22_LDR_ENUM(pLdrEntry) {
		if (pLdrEntry->DllBase == lpImageBase)
			return pLdrEntry;
	}
	return NULL;
}
