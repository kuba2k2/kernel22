// Copyright (c) Kuba Szczodrzyński 2024-2-22.

#include "kernel22.h"

static CHAR szDosStub[] = "\x0E\x1F\xBA\x0E\x00\xB4\x09\xCD"
						  "\x21\xB8\x01\x4C\xCD\x21"
						  "This program cannot be run in DOS mode."
						  "\r\r\n"
						  "\x24\x00\x00\x00\x00\x00\x00\x00";

BOOL K22PatchImportTableImpl(
	BYTE bSource,
	PIMAGE_K22_HEADER pK22Header,
	PIMAGE_NT_HEADERS3264 pNt,
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor,
	PULONGLONG pFirstThunk
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

	// backup original import directory
	pK22Header->stOrigImportDescriptor = pImportDescriptor[0];
	pK22Header->dwOrigDescriptorFT	   = pImportDescriptor[1].FirstThunk;
	pK22Header->ullOrigFirstThunk[0]   = pFirstThunk[0];
	pK22Header->ullOrigFirstThunk[1]   = pFirstThunk[1];

	// clear the import directory
	RtlZeroMemory((PVOID)pImportDescriptor, sizeof(pImportDescriptor[0]));
	pImportDescriptor[1].FirstThunk = 0;
	RtlZeroMemory((PVOID)pFirstThunk, sizeof(*pFirstThunk) * 2);

	// rebuild the first import descriptor
	// point to name in DOS header
	pImportDescriptor->Name = (ULONG_PTR)&pK22Header->szModuleName - (ULONG_PTR)pK22Header;
	// point to original FT
	pImportDescriptor->FirstThunk = pK22Header->stOrigImportDescriptor.FirstThunk;
	// overwrite the original FT, point to name in DOS header
	pFirstThunk[0] = (ULONG_PTR)&pK22Header->wSymbolHint - (ULONG_PTR)pK22Header;

	// store names in the header
	RtlCopyMemory(pK22Header->szModuleName, K22_CORE_DLL, sizeof(K22_CORE_DLL));
	RtlCopyMemory(pK22Header->szSymbolName, K22_LOAD_SYMBOL, sizeof(K22_LOAD_SYMBOL));
	pK22Header->wSymbolHint = 0;

	// backup the bound import directory
	PIMAGE_DATA_DIRECTORY pDataDirectory;
	if (pNt->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		pDataDirectory = &pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
	} else if (pNt->stNt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		pDataDirectory = &pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
	} else {
		RETURN_K22_F("Unrecognized OptionalHeader magic %04X", pNt->stNt32.OptionalHeader.Magic);
	}
	pK22Header->stOrigBoundImportDirectory = *pDataDirectory;
	pDataDirectory->VirtualAddress		   = 0;
	pDataDirectory->Size				   = 0;

	return TRUE;
}

BOOL K22RestoreImportTableImpl(
	PIMAGE_K22_HEADER pK22Header,
	PIMAGE_NT_HEADERS3264 pNt,
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor,
	PULONGLONG pFirstThunk
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

	// restore original import directory
	pImportDescriptor[0]			= pK22Header->stOrigImportDescriptor;
	pImportDescriptor[1].FirstThunk = pK22Header->dwOrigDescriptorFT;
	pFirstThunk[0]					= pK22Header->ullOrigFirstThunk[0];
	pFirstThunk[1]					= pK22Header->ullOrigFirstThunk[1];

	// clear the header
	RtlZeroMemory(&pK22Header->stOrigImportDescriptor, sizeof(pK22Header->stOrigImportDescriptor));
	pK22Header->dwOrigDescriptorFT = 0;
	RtlZeroMemory(pK22Header->ullOrigFirstThunk, sizeof(*pK22Header->ullOrigFirstThunk) * 2);
	RtlZeroMemory(pK22Header->szModuleName, sizeof(K22_CORE_DLL));
	RtlZeroMemory(pK22Header->szSymbolName, sizeof(K22_LOAD_SYMBOL));
	pK22Header->wSymbolHint = 0;

	// restore the bound import directory
	PIMAGE_DATA_DIRECTORY pDataDirectory;
	if (pNt->stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		pDataDirectory = &pNt->stNt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
	} else if (pNt->stNt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		pDataDirectory = &pNt->stNt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
	} else {
		RETURN_K22_F("Unrecognized OptionalHeader magic %04X", pNt->stNt32.OptionalHeader.Magic);
	}
	*pDataDirectory										  = pK22Header->stOrigBoundImportDirectory;
	pK22Header->stOrigBoundImportDirectory.VirtualAddress = 0;
	pK22Header->stOrigBoundImportDirectory.Size			  = 0;

	// write back a dummy DOS stub
	RtlCopyMemory(pK22Header->bDosStub, szDosStub, sizeof(pK22Header->bDosStub));

	return TRUE;
}
