// Copyright (c) Kuba Szczodrzyński 2024-2-25.

#include "kernel22.h"

static LPSTR K22LoadReasonString(LDR_DLL_LOAD_REASON eLoadReason) {
	switch (eLoadReason) {
		case LoadReasonStaticDependency:
			return "LoadReasonStaticDependency";
		case LoadReasonStaticForwarderDependency:
			return "LoadReasonStaticForwarderDependency";
		case LoadReasonDynamicForwarderDependency:
			return "LoadReasonDynamicForwarderDependency";
		case LoadReasonDelayloadDependency:
			return "LoadReasonDelayloadDependency";
		case LoadReasonDynamicLoad:
			return "LoadReasonDynamicLoad";
		case LoadReasonAsImageLoad:
			return "LoadReasonAsImageLoad";
		case LoadReasonAsDataLoad:
			return "LoadReasonAsDataLoad";
		case LoadReasonEnclavePrimary:
			return "LoadReasonEnclavePrimary";
		case LoadReasonEnclaveDependency:
			return "LoadReasonEnclaveDependency";
		case LoadReasonUnknown:
			return "LoadReasonUnknown";
	}
	return "???";
}

VOID K22DebugPrintModules() {
	K22_D("InLoadOrderModuleList:");
	K22_LDR_ENUM(pLdrEntry, InLoadOrderModuleList, InLoadOrderLinks) {
		K22_D(
			"DLL @ %p: %ls (size %lu) - entry @ %p",
			pLdrEntry->DllBase,
			pLdrEntry->BaseDllName.Buffer,
			pLdrEntry->SizeOfImage,
			pLdrEntry->EntryPoint
		);
	}
	K22_D("InInitializationOrderModuleList:");
	K22_LDR_ENUM(pLdrEntry, InInitializationOrderModuleList, InInitializationOrderLinks) {
		K22_D(
			"DLL @ %p: %ls (size %lu) - entry @ %p",
			pLdrEntry->DllBase,
			pLdrEntry->BaseDllName.Buffer,
			pLdrEntry->SizeOfImage,
			pLdrEntry->EntryPoint
		);
	}
}

BOOL K22DebugDumpModules(LPCSTR lpOutputDirName, LPVOID lpImageBase) {
	CreateDirectory(lpOutputDirName, NULL);
	K22_LDR_ENUM(pLdrEntry, InLoadOrderModuleList, InLoadOrderLinks) {
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
	K22_LDR_ENUM(pLdrEntry, InLoadOrderModuleList, InLoadOrderLinks) {
		if (pLdrEntry->DllBase == lpImageBase)
			return pLdrEntry;
	}
	return NULL;
}
