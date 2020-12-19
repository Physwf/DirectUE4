#pragma once

#include "UnrealMath.h"

class FD3D11Texture2D;

class FRenderTarget
{
public:
	virtual ~FRenderTarget() {}

	virtual const std::shared_ptr<FD3D11Texture2D>& GetRenderTargetTexture() const;
	//virtual FUnorderedAccessViewRHIRef GetRenderTargetUAV() const;

	virtual FIntPoint GetSizeXY() const = 0;

	/**
	* @return display gamma expected for rendering to this render target
	*/
	virtual float GetDisplayGamma() const;
protected:

	std::shared_ptr<FD3D11Texture2D> RenderTargetTextureRHI;
};
class FViewport : public FRenderTarget
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
		FIntPoint Size = GetSizeXY();
		return (float)Size.X / (float)Size.Y;
	}

	void SetSizeXY(uint32 InSizeX, uint32 InSizeY) { SizeX = InSizeX; SizeY = InSizeY; }
	virtual FIntPoint GetSizeXY() const override { return FIntPoint(SizeX, SizeY); }
	FIntPoint GetInitialPositionXY() const { return FIntPoint(InitialPositionX, InitialPositionY); }

	void InitRHI();
	void ReleaseRHI();
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

extern FViewport GWindowViewport;
