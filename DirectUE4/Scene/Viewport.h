#pragma once

#include "UnrealMath.h"

class Viewport
{
public:
	void OnKeyDown(unsigned int KeyCode);
	void OnKeyUp(unsigned int KeyCode);
	void OnMouseDown(int X, int Y);
	void OnMouseUp(int X, int Y);
	void OnRightMouseDown(int X, int Y);
	void OnRightMouseUp(int X, int Y);
	void OnMouseMove(int X, int Y);

	void Draw(bool bShouldPresent = true);
	float GetDesiredAspectRatio() const
	{
		IntPoint Size = GetSizeXY();
		return (float)Size.X / (float)Size.Y;
	}

	void SetSizeXY(uint32 InSizeX, uint32 InSizeY) { SizeX = InSizeX; SizeY = InSizeY; }
	IntPoint GetSizeXY() const { return IntPoint(SizeX, SizeY); }
	IntPoint GetInitialPositionXY() const { return IntPoint(InitialPositionX, InitialPositionY); }
private:
	/** The initial position of the viewport. */
	uint32 InitialPositionX;
	/** The initial position of the viewport. */
	uint32 InitialPositionY;
	/** The width of the viewport. */
	uint32 SizeX;
	/** The height of the viewport. */
	uint32 SizeY;
};

extern Viewport GWindowViewport;
