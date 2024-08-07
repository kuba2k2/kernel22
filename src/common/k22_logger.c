// Copyright (c) Kuba Szczodrzyński 2024-2-13.

#include "k22_logger.h"

#define COLOR_FMT			 "\e[0;30m"
#define COLOR_BLACK			 0x00
#define COLOR_RED			 0x01
#define COLOR_GREEN			 0x02
#define COLOR_YELLOW		 0x03
#define COLOR_BLUE			 0x04
#define COLOR_MAGENTA		 0x05
#define COLOR_CYAN			 0x06
#define COLOR_WHITE			 0x07
#define COLOR_BRIGHT_BLACK	 0x10
#define COLOR_BRIGHT_RED	 0x11
#define COLOR_BRIGHT_GREEN	 0x12
#define COLOR_BRIGHT_YELLOW	 0x13
#define COLOR_BRIGHT_BLUE	 0x14
#define COLOR_BRIGHT_MAGENTA 0x15
#define COLOR_BRIGHT_CYAN	 0x16
#define COLOR_BRIGHT_WHITE	 0x17

static TCHAR acLevels[] = {'T', 'V', 'D', 'I', 'W', 'E', 'F'};

#if K22_LOGGER_COLOR
static TCHAR adwColors[] = {
	COLOR_BRIGHT_CYAN,
	COLOR_BRIGHT_CYAN,
	COLOR_BRIGHT_BLUE,
	COLOR_BRIGHT_GREEN,
	COLOR_BRIGHT_YELLOW,
	COLOR_BRIGHT_RED,
	COLOR_BRIGHT_MAGENTA,
};
#endif

static CHAR pMessageBuffer[1024];
static PCHAR pMessageHead = pMessageBuffer;
static BOOL K22AppendError(LPCSTR lpMessage);

static VOID K22OutputMessage() {
	if (pMessageHead == pMessageBuffer)
		return;
	// end message with a newline character
	pMessageHead[0] = '\n';
	pMessageHead[1] = '\0';
	// send a string to the debugger
#if K22_LOG_OUTPUT_DEBUG_STRING
	OutputDebugString(pMessageBuffer);
#endif
	// print the message to console (verifier can't use console)
#if !K22_VERIFIER
	// don't print to console if debugged via the loader (it prints debug strings anyway)
	static DWORD dwUsePrintf = -1;
	if (dwUsePrintf == -1) {
		dwUsePrintf = !(NtCurrentPeb()->BeingDebugged && (ULONG_PTR)NtCurrentPeb()->pUnused == K22_SOURCE_LOADER);
	}
	if (dwUsePrintf)
		printf("%s", pMessageBuffer);
#endif
	// rewind the writing head and reset the message buffer
	pMessageBuffer[0] = '\0';
	pMessageHead	  = pMessageBuffer;
}

static DWORD K22VPrintf(LPCSTR lpFormat, va_list Args) {
	static int (*nt_vsnprintf)(char *Dest, size_t Count, const char *Format, va_list Args) = NULL;

	if (nt_vsnprintf == NULL) {
		HMODULE hNtdll = GetModuleHandle("ntdll.dll");
		nt_vsnprintf   = (void *)GetProcAddress(hNtdll, "_vsnprintf");
	}

	DWORD cbLeft	= sizeof(pMessageBuffer) - (pMessageHead - pMessageBuffer) - 2;
	DWORD cbMessage = nt_vsnprintf(pMessageHead, cbLeft, lpFormat, Args);
	pMessageHead += MIN(cbMessage, cbLeft);
	return cbMessage;
}

static DWORD K22Printf(LPCSTR lpFormat, ...) {
	va_list va_args;
	va_start(va_args, lpFormat);
	DWORD dwMessage = K22VPrintf(lpFormat, va_args);
	va_end(va_args);
	return dwMessage;
}

VOID K22LogWrite(
	DWORD dwLevel,
	LPCSTR lpFile,
	DWORD dwLine,
	LPCSTR lpFunction,
	DWORD dwWin32Error,
	LPCSTR lpFormat,
	...
) {
	if (pK22Data && dwLevel < pK22Data->stConfig.dwLogLevel)
		return;

#if K22_LOGGER_COLOR
	TCHAR cBright = (TCHAR)('0' + (adwColors[dwLevel] >> 4));
	TCHAR cValue  = (TCHAR)('0' + (adwColors[dwLevel] & 0x7));
#endif

#if K22_LOGGER_TIMESTAMP
	SYSTEMTIME SystemTime = {2000, 1, 1, 1, 0, 0, 0, 0};
#if !K22_VERIFIER
	// for some reason this never exits if used in verifier
	GetLocalTime(&SystemTime);
#endif
#endif

#if K22_LOGGER_FILE
	if (lpFile) {
		LPCSTR lpFileEnd = lpFile;
		while (*(++lpFileEnd) != '\0') {}
		while (*(--lpFileEnd) != '\\') {}
		lpFile = lpFileEnd + 1;
	}
#endif

	DWORD cbMessagePrefix = K22Printf(
	// format:
#if K22_LOGGER_COLOR
		"\x1B[%c;3%cm"
#endif
		"%c "
#if K22_LOGGER_TIMESTAMP
		"[%04d-%02d-%02d %02d:%02d:%02d.%03d] "
#endif
#if K22_LOGGER_COLOR
		"\x1B[0m"
#endif
#if K22_LOGGER_FILE && K22_LOGGER_FUNCTION
		"%s:%3lu(%s): "
#elif K22_LOGGER_FILE
		"%s:%3lu: "
#elif K22_LOGGER_FUNCTION
		"%s: "
#endif
		,
	// arguments:
#if K22_LOGGER_COLOR
		cBright, // whether text is bright
		cValue,	 // text color
#endif
		acLevels[dwLevel]
#if K22_LOGGER_TIMESTAMP
		,
		SystemTime.wYear,
		SystemTime.wMonth,
		SystemTime.wDay,
		SystemTime.wHour,
		SystemTime.wMinute,
		SystemTime.wSecond,
		SystemTime.wMilliseconds
#endif
#if K22_LOGGER_FILE
		,
		lpFile,
		dwLine
#endif
#if K22_LOGGER_FUNCTION
		,
		lpFunction
#endif
	);

	LPCSTR lpMessageOnly = pMessageHead;
	va_list va_args;
	va_start(va_args, lpFormat);
	K22VPrintf(lpFormat, va_args);
	va_end(va_args);
	if (dwLevel >= K22_LEVEL_ERROR)
		K22AppendError(lpMessageOnly);
	K22OutputMessage();

	if (dwWin32Error == ERROR_SUCCESS)
		return;

#if K22_LOGGER_COLOR
	// subtract color codes
	cbMessagePrefix -= 11;
#endif
	// subtract error message prefix length to make it align nicely
	cbMessagePrefix -= 12;

	LPSTR lpMessage;
	if (dwWin32Error == STATUS_BREAKPOINT) {
		lpMessage = "Debugger breakpoint";
	} else {
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwWin32Error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&lpMessage,
			0,
			NULL
		);
		if (lpMessage) {
			// strip newlines and whitespace
			LPSTR lpMessagePtr = lpMessage;
			while (*(++lpMessagePtr)) {
				if (*lpMessagePtr == '\r' || *lpMessagePtr == '\n') {
					*lpMessagePtr = ' ';
				}
			}
			while (*(--lpMessagePtr) == ' ') {}
			lpMessagePtr[1] = '\0';
		}
	}

	lpMessageOnly = pMessageHead;
	K22Printf("%*c====> CODE: %s (0x%08lx)", cbMessagePrefix, ' ', lpMessage, dwWin32Error);
	if (dwLevel >= K22_LEVEL_ERROR)
		K22AppendError(lpMessageOnly);
	K22OutputMessage();

	if (dwWin32Error != STATUS_BREAKPOINT) {
		LocalFree(lpMessage);
	}
}

typedef struct K22_ERROR {
	LPSTR lpMessage;
	DWORD cchMessage;
	struct K22_ERROR *pNext;
	struct K22_ERROR *pPrev;
} K22_ERROR, *PK22_ERROR;

static PK22_ERROR pErrors = NULL;

static BOOL K22AppendError(LPCSTR lpMessage) {
	PK22_ERROR pError;
	pError = malloc(sizeof(*pError));
	if (pError == NULL)
		return FALSE;
	memset(pError, 0, sizeof(*pError));
	K22_LL_APPEND(pErrors, pError);
	pError->cchMessage = strlen(lpMessage);
	K22StringDup(lpMessage, pError->cchMessage, &pError->lpMessage);
	return TRUE;
}

LPSTR K22LogGetErrors(LPCSTR lpPrefix) {
	// count the total length of messages
	DWORD cchPrefix = strlen(lpPrefix);
	DWORD cchErrors = cchPrefix;
	PK22_ERROR pError, pTmp;
	K22_LL_FOREACH(pErrors, pError) {
		cchErrors += pError->cchMessage + sizeof("\r\n") - 1;
	}
	// allocate a string
	LPSTR lpErrors;
	K22_MALLOC_LENGTH(lpErrors, cchErrors + 1);
	// copy the prefix message
	memcpy(lpErrors, lpPrefix, cchPrefix + 1);
	// copy each message while deleting them
	LPSTR lpWriteHead = lpErrors + cchPrefix;
	K22_LL_FOREACH_SAFE(pErrors, pError) {
		strcpy(lpWriteHead, pError->lpMessage);
		lpWriteHead += pError->cchMessage;
		strcpy(lpWriteHead, "\r\n");
		lpWriteHead += sizeof("\r\n") - 1;
		K22_LL_DELETE(pErrors, pError);
		K22_FREE(pError->lpMessage);
		K22_FREE(pError);
	}
	return lpErrors;
}

VOID K22LogShowErrorMessage(LPCSTR lpPrefix) {
	LPSTR lpErrors = K22LogGetErrors(lpPrefix);
	MessageBox(0, lpErrors, "Error", MB_ICONERROR);
	free(lpErrors);
}
