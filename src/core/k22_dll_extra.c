// Copyright (c) Kuba Szczodrzyński 2024-8-4.

#include "kernel22.h"

BOOL K22DllExtraLoadAll() {
	PK22_DLL_EXTRA pDllExtra = NULL;
	K22_LL_FOREACH(pK22Data->stDll.pDllExtra, pDllExtra) {
		if (pDllExtra->hModule != NULL)
			continue;
		K22_I("DLL Extra: loading %s (%s)", pDllExtra->lpKey, pDllExtra->lpTargetDll);
		pDllExtra->hModule = LoadLibrary(pDllExtra->lpTargetDll);
		if (pDllExtra->hModule == NULL)
			RETURN_K22_F_ERR("Couldn't load extra DLL - %s", pDllExtra->lpTargetDll);
	}
	return TRUE;
}
