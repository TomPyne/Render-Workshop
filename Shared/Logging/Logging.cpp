#include "Logging.h"

#include <cassert>
#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>

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

void LogError(const char* str)
{
	OutputDebugStringA(str);
	__debugbreak();
}

void LogWarning(const char* str)
{
	OutputDebugStringA(str);
}

void LogInfo(const char* str)
{
	OutputDebugStringA(str);
}

void LogDebug(const char* str)
{
	OutputDebugStringA(str);
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
