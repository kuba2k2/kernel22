// Copyright (c) Kuba Szczodrzyński 2024-8-4.

#include "winver.h"

PWIN_VER_ENTRY pWinVerEntries = NULL;
DWORD fWinVerModes			  = 0;
static BOOL bFoundDefault	  = FALSE;

static WIN_VER_MODE StringToMode(LPCSTR lpModeName) {
#define CHECK_MODE(name)                                                                                               \
	if (_stricmp(lpModeName, #name) == 0)                                                                              \
		return (name);
	if (lpModeName && *lpModeName) {
		CHECK_MODE(GETVERSION);
		CHECK_MODE(GETVERSIONEXA);
		CHECK_MODE(GETVERSIONEXW);
		CHECK_MODE(RTLGETVERSION);
		CHECK_MODE(RTLGETNTVERSIONNUMBERS);
		CHECK_MODE(VERQUERYVALUEA);
		CHECK_MODE(VERQUERYVALUEW);
	}
	return 0;
#undef CHECK_MODE
}

static LPCSTR ModeToString(WIN_VER_MODE eMode) {
#define CHECK_MODE(name)                                                                                               \
	case name:                                                                                                         \
		return #name;
	switch (eMode) {
		CHECK_MODE(GETVERSION);
		CHECK_MODE(GETVERSIONEXA);
		CHECK_MODE(GETVERSIONEXW);
		CHECK_MODE(RTLGETVERSION);
		CHECK_MODE(RTLGETNTVERSIONNUMBERS);
		CHECK_MODE(VERQUERYVALUEA);
		CHECK_MODE(VERQUERYVALUEW);
		case MODE_MATCH_ANY:
			return "*";
	}
	return "?";
#undef CHECK_MODE
}

static VOID ParseVersion(LPCSTR lpWinVer, PWIN_VER_ENTRY pWinVerEntry) {
	if (lpWinVer == NULL || lpWinVer[0] == '\0') {
		pWinVerEntry->dwMajor = NtCurrentPeb()->OSMajorVersion;
		pWinVerEntry->dwMinor = NtCurrentPeb()->OSMinorVersion;
		pWinVerEntry->dwBuild = NtCurrentPeb()->OSBuildNumber;
	} else {
		sscanf(lpWinVer, "%ld.%ld.%ld", &pWinVerEntry->dwMajor, &pWinVerEntry->dwMinor, &pWinVerEntry->dwBuild);
	}
}

BOOL WinVerParseConfig(HKEY hWinVer) {
	K22_REG_VARS();

	if (K22_REG_READ_VALUE(hWinVer, "ModeFlags", &fWinVerModes, cbValue)) {
		K22_D("WinVer mode: %08lx", fWinVerModes);
	}

	K22_REG_ENUM_VALUE(hWinVer, szName, cbName, szValue, cbValue) {
		if (_stricmp(szName, "ModeFlags") == 0) // skip ModeFlags value
			continue;
		LPSTR lpModuleName = NULL;
		WIN_VER_MODE eMode = MODE_MATCH_ANY;
		LPSTR lpSeparator  = NULL;
		if ((lpSeparator = strchr(szName, ';')) != NULL) {
			// found module;mode separator
			*lpSeparator = '\0';
			lpModuleName = szName;
			eMode		 = StringToMode(lpSeparator + 1);
		} else if (strchr(szName, '.') != NULL) {
			// found file extension separator
			lpModuleName = szName;
		} else if (szName[0] != '\0') {
			// no separator, but string not empty
			eMode = StringToMode(szName);
		} else {
			// default value from registry
			bFoundDefault = TRUE;
		}

		PWIN_VER_ENTRY pWinVerEntry;
		K22_LL_FIND(
			pWinVerEntries,
			pWinVerEntry,
			(
				// compare by module name (can be null)
				_stricmp(
					lpModuleName ? lpModuleName : "",
					pWinVerEntry->lpModuleName ? pWinVerEntry->lpModuleName : ""
				) == 0
			) && eMode == pWinVerEntry->eMode
		);

		if (pWinVerEntry == NULL) {
			K22_LL_ALLOC_APPEND(pWinVerEntries, pWinVerEntry);
			if (lpModuleName && !K22StringDup(lpModuleName, strlen(lpModuleName), &pWinVerEntry->lpModuleName))
				return FALSE;
			pWinVerEntry->eMode = eMode;
		} else {
			K22_V(
				"WinVer: will replace %s/%s",
				pWinVerEntry->lpModuleName ? pWinVerEntry->lpModuleName : "*",
				ModeToString(pWinVerEntry->eMode)
			);
		}

		ParseVersion(szValue, pWinVerEntry);

		K22_D(
			"WinVer: setting %ld.%ld.%ld for %s/%s",
			pWinVerEntry->dwMajor,
			pWinVerEntry->dwMinor,
			pWinVerEntry->dwBuild,
			pWinVerEntry->lpModuleName ? pWinVerEntry->lpModuleName : "*",
			ModeToString(pWinVerEntry->eMode)
		);
	}

	if (!bFoundDefault) {
		// add real version as default, if not set before
		PWIN_VER_ENTRY pWinVerEntry;
		K22_LL_ALLOC_APPEND(pWinVerEntries, pWinVerEntry);
		ParseVersion(NULL, pWinVerEntry);
		K22_D(
			"WinVer: setting %ld.%ld.%ld as default",
			pWinVerEntry->dwMajor,
			pWinVerEntry->dwMinor,
			pWinVerEntry->dwBuild
		);
	}

	return TRUE;
}

PWIN_VER_ENTRY WinVerGetConfig(LPCSTR lpModuleName, WIN_VER_MODE eMode) {
	// find a matching entry
	PWIN_VER_ENTRY pWinVerEntry, pModuleMatch = NULL, pModeMatch = NULL, pDefault = NULL;
	K22_LL_FOREACH(pWinVerEntries, pWinVerEntry) {
		BOOL bModuleMatch = pWinVerEntry->lpModuleName && _stricmp(pWinVerEntry->lpModuleName, lpModuleName) == 0;
		BOOL bModeMatch	  = pWinVerEntry->eMode == eMode;
		if (bModuleMatch && bModeMatch)
			return pWinVerEntry;
		if (bModuleMatch && pWinVerEntry->eMode == MODE_MATCH_ANY)
			pModuleMatch = pWinVerEntry;
		else if (bModeMatch && pWinVerEntry->lpModuleName == NULL)
			pModeMatch = pWinVerEntry;
		else if (pWinVerEntry->eMode == MODE_MATCH_ANY && pWinVerEntry->lpModuleName == NULL)
			pDefault = pWinVerEntry;
	}
	// pModuleMatch -> pModeMatch -> pDefault
	return pModuleMatch ? pModuleMatch : pModeMatch ? pModeMatch : pDefault;
}
