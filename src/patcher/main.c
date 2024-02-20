// Copyright (c) Kuba Szczodrzyński 2024-2-20.

#include "kernel22.h"

static BYTE abInitCode64[53] =
	//
	"\x55"													   // push rbp
	"\x48\x89\xe5"											   // mov rbp, rsp
	"\x48\x83\xec\x20"										   // sub rsp, 20h
	"\x48\x8d\x0d\x02\x00\x00\x00"							   // lea rcx, [rip + 2]
	"\xeb\x0e"												   // jmp 2 + 13 + 1
	"\x4b\x32\x32\x43\x6f\x72\x65\x36\x34\x2e\x64\x6c\x6c\x00" // .string "K22Core64.dll"
	"\x48\xb8\x40\x61\x51\x77\x00\x00\x00\x00"				   // movabs rax, 0x77516140
	"\xff\xd0"												   // call rax
	"\x48\x83\xc4\x20"										   // add rsp, 20h
	"\x5d"													   // pop rbp
	"\xe9\xfa\xff\xff\xff";									   // jmp 0xFFFFFFFF

#define CODE64_JUMP_SIZE				(5)
#define CODE64_JUMP_OFFS				(sizeof(abInitCode64) - CODE64_JUMP_SIZE)
#define CODE64_LOAD_LIBRARY_ADDR(pCode) (*(PULONGLONG)((PBYTE)pCode + 33))
#define CODE64_JUMP_ADDR(pCode)			(*(PDWORD)((PBYTE)pCode + CODE64_JUMP_OFFS + 1))

BOOL PatcherMain(HANDLE hFile) {
	DWORD dwFileBytes;

	// read DOS header
	IMAGE_DOS_HEADER stDosHeader;
	if (!K22ReadFile(hFile, 0, stDosHeader, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't read DOS header");
	// read NT header
	IMAGE_NT_HEADERS3264 stNt;
	if (!K22ReadFile(hFile, stDosHeader.e_lfanew, stNt, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't read NT headers");

	// disable the import table
	if (!K22PatchImportTable(&stDosHeader, &stNt, K22_PATCHER_FILE))
		return FALSE;

	// get patch code depending on bitness
	BOOL fIs64Bit = stNt.stNt64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
	DWORD cbPatchSize;
	PBYTE pPatchCode;
	if (fIs64Bit) {
		cbPatchSize = sizeof(abInitCode64);
		pPatchCode	= abInitCode64;
	} else {
		RETURN_K22_F("Patching 32-bit images is not implemented yet");
	}

	// find section headers
	SIZE_T lSectionsOffset =
		stDosHeader.e_lfanew + sizeof(stNt.dwSignature) + sizeof(stNt.stFile) + stNt.stFile.SizeOfOptionalHeader;
	SIZE_T cbSections = stNt.stFile.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
	// read section headers
	PIMAGE_SECTION_HEADER pSections = LocalAlloc(0, cbSections);
	if (pSections == NULL)
		RETURN_K22_F_ERR("Couldn't allocate section headers");
	if (!K22ReadFileLength(hFile, lSectionsOffset, pSections, cbSections, &dwFileBytes)) {
		K22_F_ERR("Couldn't read section headers");
		LocalFree(pSections);
		return FALSE;
	}

	IMAGE_SECTION_HEADER stTargetSection = {
		.Characteristics = 0,
	};
	for (PIMAGE_SECTION_HEADER pSection = pSections; pSection < pSections + stNt.stFile.NumberOfSections; pSection++) {
		if (!(pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE))
			continue;
		DWORD dwRawSize		= pSection->SizeOfRawData;
		DWORD dwVirtualSize = pSection->Misc.VirtualSize;
		if (dwVirtualSize >= dwRawSize)
			continue;
		DWORD dwFreeSize = dwRawSize - dwVirtualSize;
		K22_D(
			"Section %s @ RVA %08lX, raw=%lX, virt=%lX, free=%lX, patch=%lX",
			pSection->Name,
			pSection->VirtualAddress,
			dwRawSize,
			dwVirtualSize,
			dwFreeSize,
			cbPatchSize
		);
		if (dwFreeSize < cbPatchSize)
			continue;
		stTargetSection = *pSection;
		break;
	}
	LocalFree(pSections);

	if (stTargetSection.Characteristics == 0)
		RETURN_K22_F("Couldn't find a suitable code section");

	// calculate physical & virtual offsets
	DWORD dwPatchOffset	  = stTargetSection.PointerToRawData + stTargetSection.Misc.VirtualSize;
	DWORD dwPatchEntry	  = stTargetSection.VirtualAddress + stTargetSection.Misc.VirtualSize;
	DWORD dwOriginalEntry = stNt.stNt64.OptionalHeader.AddressOfEntryPoint; // 32/64-bit have the same offset

	if (fIs64Bit) {
		CODE64_LOAD_LIBRARY_ADDR(pPatchCode) = (ULONGLONG)LoadLibraryA;
		CODE64_JUMP_ADDR(pPatchCode)		 = dwOriginalEntry - dwPatchEntry - CODE64_JUMP_OFFS - CODE64_JUMP_SIZE;
	} else {
	}

	// write patch code
	K22_I("Writing K22 patch @ %lX, RVA %lX", dwPatchOffset, dwPatchEntry);
	if (!K22WriteFileLength(hProcess, dwPatchOffset, pPatchCode, cbPatchSize, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write patch code");
	// change entrypoint
	stNt.stNt64.OptionalHeader.AddressOfEntryPoint = dwPatchEntry;

	// write modified DOS header
	if (!K22WriteFile(hProcess, 0, stDosHeader, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write DOS header");
	// write modified NT header
	if (!K22WriteFile(hProcess, stDosHeader.e_lfanew, stNt, &dwFileBytes))
		RETURN_K22_F_ERR("Couldn't write NT header");

	return TRUE;
}

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		printf("usage: %s <filename>\n", argv[0]);
		return 1;
	}

	LPCSTR lpImageName = argv[1];

	HANDLE hFile =
		CreateFile(lpImageName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		RETURN_K22_F_ERR("Couldn't open the file '%s'", lpImageName);

	BOOL fResult = PatcherMain(hFile);
	CloseHandle(hFile);

	return !fResult;
}
