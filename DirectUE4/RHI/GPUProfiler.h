#pragma once

#include "UnrealMath.h"

struct GPUProfiler
{
	GPUProfiler() {}

	void PushEvent(const wchar_t* Name);
	void PopEvent();

	void BeginFrame();
	void EndFrame();
private:
};

struct ScopedDrawEvent
{
	~ScopedDrawEvent()
	{
		Stop();
	}
	void Start(const wchar_t* Fmt, ...);
	void Stop();
};

#define PREPROCESSOR_JOIN(x, y) PREPROCESSOR_JOIN_INNER(x, y)
#define PREPROCESSOR_JOIN_INNER(x, y) x##y

#define SCOPED_DRAW_EVENT(Name) ScopedDrawEvent PREPROCESSOR_JOIN(Event_##Name,__LINE__); PREPROCESSOR_JOIN(Event_##Name,__LINE__).Start(TEXT(#Name));
#define SCOPED_DRAW_EVENT_FORMAT(Name, Format, ...) ScopedDrawEvent PREPROCESSOR_JOIN(Event_##Name,__LINE__); PREPROCESSOR_JOIN(Event_##Name,__LINE__).Start(Format, ##__VA_ARGS__);