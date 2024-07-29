// Copyright (c) Kuba Szczodrzyński 2024-2-22.

#include "kernel22.h"

BOOL K22PatchImportTableImpl(
	BYTE bSource, PIMAGE_K22_HEADER pK22Header, PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor, PULONGLONG pFirstThunk
) {
	// make sure there's enough room in the DOS stub
	if (pK22Header->dwPeRva < sizeof(*pK22Header))
		RETURN_K22_F(
			"PE header overlaps DOS stub! Not enough free space. (0x%X < 0x%X)",
			pK22Header->dwPeRva,
			sizeof(*pK22Header)
		);

	// always set the latest load source
	pK22Header->bSource = bSource;
	// exit if the image has been patched
	if (RtlCompareMemory(pK22Header->bCookie, K22_COOKIE, 3) == 3) {
		K22_W("Image has already been patched!");
		return TRUE;
	}
	// otherwise write the patch cookie
	RtlCopyMemory(pK22Header->bCookie, K22_COOKIE, 3);

	// copy original import directory
	pK22Header->stOrigImportDescriptor[0] = pImportDescriptor[0];
	pK22Header->stOrigImportDescriptor[1] = pImportDescriptor[1];
	pK22Header->ullOrigFirstThunk[0]	  = pFirstThunk[0];
	pK22Header->ullOrigFirstThunk[1]	  = pFirstThunk[1];

	// clear the import directory
	RtlZeroMemory((PVOID)pImportDescriptor, sizeof(*pImportDescriptor) * 2);
	RtlZeroMemory((PVOID)pFirstThunk, sizeof(*pFirstThunk) * 2);

	// rebuild the first import descriptor
	// point to name in DOS header
	pImportDescriptor->Name = (ULONG_PTR)&pK22Header->szModuleName - (ULONG_PTR)pK22Header;
	// point to original FT
	pImportDescriptor->FirstThunk = pK22Header->stOrigImportDescriptor[0].FirstThunk;
	// overwrite the original FT, point to name in DOS header
	pFirstThunk[0] = (ULONG_PTR)&pK22Header->wSymbolHint - (ULONG_PTR)pK22Header;

	// store names in the header
	RtlCopyMemory(pK22Header->szModuleName, K22_CORE_DLL, sizeof(K22_CORE_DLL));
	RtlCopyMemory(pK22Header->szSymbolName, K22_LOAD_SYMBOL, sizeof(K22_LOAD_SYMBOL));
	pK22Header->wSymbolHint = 0;

	return TRUE;
}

BOOL K22RestoreImportTableImpl(
	PIMAGE_K22_HEADER pK22Header, PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor, PULONGLONG pFirstThunk
) {
	// make sure there's enough room in the DOS stub
	if (pK22Header->dwPeRva < sizeof(*pK22Header))
		RETURN_K22_F(
			"PE header overlaps DOS stub! Not enough free space. (0x%X < 0x%X)",
			pK22Header->dwPeRva,
			sizeof(*pK22Header)
		);

	// exit if the image has been patched
	if (RtlCompareMemory(pK22Header->bCookie, K22_COOKIE, 3) != 3) {
		K22_W("Image has NOT been patched before!");
		return TRUE;
	}
	// otherwise clear the patch cookie
	RtlZeroMemory(pK22Header->bCookie, 3);
	// clear the load source
	pK22Header->bSource = K22_SOURCE_NONE;

	// restore the import directory
	pImportDescriptor[0] = pK22Header->stOrigImportDescriptor[0];
	pImportDescriptor[1] = pK22Header->stOrigImportDescriptor[1];
	pFirstThunk[0]		 = pK22Header->ullOrigFirstThunk[0];
	pFirstThunk[1]		 = pK22Header->ullOrigFirstThunk[1];

	// clear the header
	RtlZeroMemory(pK22Header->stOrigImportDescriptor, sizeof(*pK22Header->stOrigImportDescriptor) * 2);
	RtlZeroMemory(pK22Header->ullOrigFirstThunk, sizeof(*pK22Header->ullOrigFirstThunk) * 2);
	RtlZeroMemory(pK22Header->szModuleName, sizeof(K22_CORE_DLL));
	RtlZeroMemory(pK22Header->szSymbolName, sizeof(K22_LOAD_SYMBOL));
	pK22Header->wSymbolHint = 0;

	// write back a dummy DOS stub
	RtlCopyMemory(
		pK22Header->bDosStub1,
		"\x0E\x1F\xBA\x0E\x00\xB4\x09\xCD\x21\xB8\x01\x4C\xCD\x21This program cannot be run in DOS mode.\r\r\n",
		sizeof(pK22Header->bDosStub1)
	);

	return TRUE;
}

BOOL K22PatchBoundImportTableImpl(PIMAGE_K22_HEADER pK22Header, PIMAGE_NT_HEADERS3264 pNt) {
	// copy and patch the bound import directory
	if (pNt->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		pK22Header->stOrigBoundImportDirectory =
			pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
		pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
		pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size			= 0;
	} else if (pNt->stNt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		pK22Header->stOrigBoundImportDirectory =
			pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
		pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
		pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size			= 0;
	} else {
		RETURN_K22_F("Unrecognized OptionalHeader magic %04X", pNt->stNt32.OptionalHeader.Magic);
	}
	return TRUE;
}

BOOL K22RestoreBoundImportTableImpl(PIMAGE_K22_HEADER pK22Header, PIMAGE_NT_HEADERS3264 pNt) {
	// restore the bound import directory
	if (pNt->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] =
			pK22Header->stOrigBoundImportDirectory;
	} else if (pNt->stNt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] =
			pK22Header->stOrigBoundImportDirectory;
	} else {
		RETURN_K22_F("Unrecognized OptionalHeader magic %04X", pNt->stNt32.OptionalHeader.Magic);
	}

	// clear the header
	pK22Header->stOrigBoundImportDirectory.VirtualAddress = 0;
	pK22Header->stOrigBoundImportDirectory.Size			  = 0;

	// write back a dummy DOS stub
	RtlCopyMemory(pK22Header->bDosStub2, "\x24\x00\x00\x00\x00\x00\x00\x00", sizeof(pK22Header->bDosStub2));

	return TRUE;
}
