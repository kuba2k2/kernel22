// Copyright (c) Kuba Szczodrzyński 2024-8-4.

#include "kernel22.h"

typedef enum WIN_VER_MODE {
	MODE_MATCH_ANY		   = 0,
	GETVERSION			   = (1 << 0),
	GETVERSIONEXA		   = (1 << 1),
	GETVERSIONEXW		   = (1 << 2),
	RTLGETVERSION		   = (1 << 4),
	RTLGETNTVERSIONNUMBERS = (1 << 5),
	VERQUERYVALUEA		   = (1 << 8),
	VERQUERYVALUEW		   = (1 << 9),
} WIN_VER_MODE;

typedef struct WIN_VER_ENTRY {
	LPSTR lpModuleName; // optional, module name to match
	WIN_VER_MODE eMode; // optional, spoofing mode to match
	DWORD dwMajor;		// major version number
	DWORD dwMinor;		// minor version number
	DWORD dwBuild;		// build number
	struct WIN_VER_ENTRY *pPrev;
	struct WIN_VER_ENTRY *pNext;
} WIN_VER_ENTRY, *PWIN_VER_ENTRY;

extern DWORD fWinVerModes;
extern PWIN_VER_ENTRY pWinVerEntries;

BOOL WinVerParseConfig(HKEY hWinVer);
PWIN_VER_ENTRY WinVerGetConfig(LPCSTR lpModuleName, WIN_VER_MODE eMode);

K22_HOOK_DEF(DWORD, GetVersion, ());
K22_HOOK_DEF(BOOL, GetVersionExA, (LPOSVERSIONINFOA lpVersionInformation));
K22_HOOK_DEF(BOOL, GetVersionExW, (LPOSVERSIONINFOW lpVersionInformation));
K22_HOOK_DEF(NTSTATUS, RtlGetVersion, (PRTL_OSVERSIONINFOW lpVersionInformation));
K22_HOOK_DEF(VOID, RtlGetNtVersionNumbers, (PDWORD pMajorVersion, PDWORD pMinorVersion, PDWORD pBuildNumber));
K22_HOOK_DEF(BOOL, VerQueryValueA, (LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen));
K22_HOOK_DEF(BOOL, VerQueryValueW, (LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen));
