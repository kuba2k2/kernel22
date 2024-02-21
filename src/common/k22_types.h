// Copyright (c) Kuba Szczodrzyński 2024-2-21.

#pragma once

#include "kernel22.h"

typedef struct {
	// e_res[0], e_res[1]
	BYTE abPatcherCookie[3];
	BYTE bPatcherType;
	// e_res[2], e_res[3]
	DWORD dwUnused1;

	// e_oemid, e_oeminfo
	DWORD _dwOem;

	// e_res2[0], e_res2[1]
	DWORD dwRvaImportDirectory;
	// e_res2[2], e_res2[3], ...
	DWORD dwUnused2[4];
} K22_HDR_DATA, *PK22_HDR_DATA;

#define K22_DOS_HDR_DATA(pDosHeader) ((PK22_HDR_DATA)(&((PIMAGE_DOS_HEADER)(pDosHeader))->e_res))

#define K22_PATCHER_COOKIE	"K22"
#define K22_PATCHER_PROCESS 'P'
#define K22_PATCHER_FILE	'F'
