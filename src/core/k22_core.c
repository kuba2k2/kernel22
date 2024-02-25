// Copyright (c) Kuba Szczodrzyński 2024-2-24.

#include "kernel22.h"

BOOL K22CoreMain(PIMAGE_K22_HEADER pK22Header) {
	K22_I("Kernel22 Core starting");

	if (!K22DataInitialize(pK22Header))
		return FALSE;

	K22_I("Load Source: %c", pK22Header->bSource);
	K22_I("Process Name: %s", pK22Data->lpProcessName);

	if (!K22DataReadConfig())
		return FALSE;

	if (MH_Initialize() != MH_OK)
		RETURN_K22_F("Couldn't initialize MinHook");

	return TRUE;
}
