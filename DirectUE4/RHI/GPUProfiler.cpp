#include "GPUProfiler.h"
#include <d3d9.h>
#include <stdio.h>
#include <stdarg.h>

void GPUProfiler::PushEvent(const wchar_t* Name)
{
	D3DPERF_BeginEvent(0, Name);
}

void GPUProfiler::PopEvent()
{
	D3DPERF_EndEvent();
}

GPUProfiler GGPUProfiler;

void ScopedDrawEvent::Start(const wchar_t* Fmt, ...)
{
	wchar_t buffer[1024] = { 0 };
	va_list v_list;
	va_start(v_list, Fmt);
	vswprintf_s(buffer, Fmt, v_list);
	va_end(v_list);
	GGPUProfiler.PushEvent(buffer);
}

void ScopedDrawEvent::Stop()
{
	GGPUProfiler.PopEvent();
}
