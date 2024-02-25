// Copyright (c) Kuba Szczodrzyński 2024-2-18.

#pragma once

#include "kernel22.h"

typedef union {
	struct {
		DWORD dwSignature;
		IMAGE_FILE_HEADER stFile;
	};

	IMAGE_NT_HEADERS32 stNt32;
	IMAGE_NT_HEADERS64 stNt64;
} IMAGE_NT_HEADERS3264, *PIMAGE_NT_HEADERS3264;

typedef struct {
	ULONG Flags;				  // Reserved.
	PCUNICODE_STRING FullDllName; // The full path name of the DLL module.
	PCUNICODE_STRING BaseDllName; // The base file name of the DLL module.
	PVOID DllBase;				  // A pointer to the base address for the DLL in memory.
	ULONG SizeOfImage;			  // The size of the DLL image, in bytes.
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef struct {
	ULONG Flags;				  // Reserved.
	PCUNICODE_STRING FullDllName; // The full path name of the DLL module.
	PCUNICODE_STRING BaseDllName; // The base file name of the DLL module.
	PVOID DllBase;				  // A pointer to the base address for the DLL in memory.
	ULONG SizeOfImage;			  // The size of the DLL image, in bytes.
} LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef union {
	LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
	LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

#define LDR_DLL_NOTIFICATION_REASON_LOADED	 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

typedef VOID(CALLBACK *PLDR_DLL_NOTIFICATION_FUNCTION)(
	ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context
);

NTSTATUS(NTAPI *LdrRegisterDllNotification)
(ULONG Flags, PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction, PVOID Context, PVOID *Cookie);
