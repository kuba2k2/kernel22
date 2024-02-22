// Copyright (c) Kuba Szczodrzyński 2024-2-21.

#pragma once

#include "kernel22.h"

typedef struct {
	DWORD Reserved1[7];	   // e_magic, ..., e_ovno

	BYTE bCookie[3];	   // e_res[0], LOBYTE(e_res[1])
	BYTE bSource;		   // HIBYTE(e_res[1])

	DWORD dwUnused1[1];	   // e_res[2], e_res[3]
	DWORD Reserved2;	   // e_oemid, e_oeminfo
	CHAR szModuleName[20]; // e_res2[0], ..., e_res2[10]

	DWORD dwPeRva;		   // e_lfanew

	// DOS stub
	IMAGE_IMPORT_DESCRIPTOR stOrigImportDescriptor[2];
	ULONGLONG ullOrigFirstThunk[2];
	WORD wSymbolHint;
	CHAR szSymbolName[6];
} IMAGE_K22_HEADER, *PIMAGE_K22_HEADER;

#define K22_DOS_HDR_DATA(lpImageBase) ((PIMAGE_K22_HEADER)(lpImageBase))

#define K22_COOKIE			"K22"
#define K22_SOURCE_PARENT	'H'
#define K22_SOURCE_LOADER	'L'
#define K22_SOURCE_VERIFIER 'V'
#define K22_SOURCE_PATCHER	'F'

#define K22_LOAD_SYMBOL "DllLd"

static VOID BuildBugCheck() {
	BUILD_BUG_ON(sizeof(IMAGE_K22_HEADER) != 128);
	BUILD_BUG_ON(sizeof(K22_LOAD_SYMBOL) != 6);
}
