#pragma once

void _LogFatalfLF(const char* fmt, ...);
void _LogErrorfLF(const char* fmt, ...);
void _LogWarningfLF(const char* fmt, ...);
void _LogInfofLF(const char* fmt, ...);
void _LogDebugfLF(const char* fmt, ...);
bool _EnsureMsg(bool condition, const char* fmt, ...);
void _AssertMsg(bool condition, const char* fmt, ...);
bool _FailMsg(const char* fmt, ...);

//#define FAST_ENSURES

#ifndef FAST_ENSURES
#define ENSUREMSG(x, ...)	_EnsureMsg(x, __VA_ARGS__)
#else
#define ENSUREMSG(x, ...) !!(x)
#endif

#define ASSERTMSG(x, ...)	_AssertMsg(x, __VA_ARGS__)
#define ASSERT0MSG(...)		_AssertMsg(false, __VA_ARGS__)
#define CHECK(x)			_AssertMsg(x, #x)

#define LOGERROR(...) 		_LogErrorfLF(__VA_ARGS__)
#define LOGWARNING(...) 	_LogWarningfLF(__VA_ARGS__)
#define LOGINFO(...) 		_LogInfofLF(__VA_ARGS__)
#define LOGDEBUG(...) 		_LogDebugfLF(__VA_ARGS__)

#define FAILMSG(...)		_FailMsg(__VA_ARGS__)

// Common fail conditions
#define RET_OUT_OF_MEM			FAILMSG("Out of memory!")
#define RET_INVALID_ARGS		FAILMSG("Invalid args");
#define RET_ARITHMETIC_OVERFLOW	FAILMSG("Arithmetic overflow");
#define RET_UNEXPECTED			FAILMSG("Unexpected condition");
#define RET_UNSUPPORTED			FAILMSG("Unsupported condition");

