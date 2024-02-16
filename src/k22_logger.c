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

VOID K22LogWrite(
	DWORD dwLevel, LPCSTR lpFile, DWORD dwLine, LPCSTR lpFunction, DWORD dwWin32Error, LPCSTR lpFormat, ...
) {
#if K22_LOGGER_COLOR
	TCHAR cBright = (TCHAR)('0' + (adwColors[dwLevel] >> 4));
	TCHAR cValue  = (TCHAR)('0' + (adwColors[dwLevel] & 0x7));
#endif

#if K22_LOGGER_TIMESTAMP
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
#endif

#if K22_LOGGER_FILE
	{
		LPCSTR lpFileEnd = lpFile + strlen(lpFile);
		while (*(--lpFileEnd) != '\\') {}
		lpFile = lpFileEnd + 1;
	}
#endif

	int cbMessagePrefix = printf(
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
		"%s:%lu(%s): "
#elif K22_LOGGER_FILE
		"%s:%lu: "
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

	va_list va_args;
	va_start(va_args, lpFormat);
	vprintf(lpFormat, va_args);
	va_end(va_args);
	//	putchar('\r');
	putchar('\n');

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
		{
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

	printf("%*c====> CODE: %s (0x%08lx)\n", cbMessagePrefix, ' ', lpMessage, dwWin32Error);

	if (dwWin32Error != STATUS_BREAKPOINT) {
		LocalFree(lpMessage);
	}
}
