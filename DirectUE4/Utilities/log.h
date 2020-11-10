#pragma once

#include <stdio.h>
#include <stdarg.h>

#define X_LOG(Format,...) XLOG(Format, __VA_ARGS__)

inline void XLOG(const char* format, ...)
{
	char buffer[16*1024] = { 0 };
	va_list v_list;
	va_start(v_list, format);
	vsprintf_s(buffer, format, v_list);
	va_end(v_list);
	extern void OutputDebug(const char* Format);
	OutputDebug(buffer);
}


