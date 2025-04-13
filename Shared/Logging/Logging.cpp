#include "Logging.h"

#include <cassert>
#include <stdio.h>
#include <Windows.h>
#include <iostream>

static bool ConsoleLogging = false;
static bool PrettyPrinting = false;

#define ERROR_COLOR 12
#define WARN_COLOR 14
#define INFO_COLOR 15
#define DEBUG_COLOR 8

constexpr size_t MaxLogSize = 16 * 2048;
thread_local char LogBuffer[MaxLogSize];

#define PlatformFormatLogMessageLf(fn) \
	va_list ap; \
	va_start(ap, fmt); \
	vsprintf_s(LogBuffer, fmt, ap); \
	va_end(ap); \
	strcat_s(LogBuffer, "\n"); \
	fn(LogBuffer); \

void LogFatal(const char* str)
{
	OutputDebugStringA(str);
#if _DEBUG
	__debugbreak();
#endif
	assert(0);
}

void LogOutput(const char* Str, int Color)
{
	OutputDebugStringA(Str);

	if (ConsoleLogging)
	{
		if (PrettyPrinting)
		{
			HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(Console, Color);
		}
		std::cout << Str;
	}
}

void LogError(const char* str)
{
	LogOutput(str, ERROR_COLOR);
	__debugbreak();
}

void LogWarning(const char* str)
{
	LogOutput(str, WARN_COLOR);
}

void LogInfo(const char* str)
{
	LogOutput(str, INFO_COLOR);
}

void LogDebug(const char* str)
{
	LogOutput(str, DEBUG_COLOR);
}

void LoggingEnableConsole(bool Enable)
{
	ConsoleLogging = Enable;
}

void LoggingEnablePrettyPrint(bool Enable)
{
	PrettyPrinting = Enable;
}

void _LogFatalfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogFatal);
}

void _LogErrorfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogError);
}

void _LogWarningfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogWarning);
}

void _LogInfofLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogInfo);
}

void _LogDebugfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogDebug);
}

bool _EnsureMsg(bool condition, const char * fmt, ...)
{
	if (condition == false)
	{		
		PlatformFormatLogMessageLf(LogError);
	}
	return condition;
}

void _AssertMsg(bool condition, const char* fmt, ...)
{
	if (condition == false)
	{
		PlatformFormatLogMessageLf(LogFatal);
	}
}

bool _FailMsg(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogError);
	return false;
}
