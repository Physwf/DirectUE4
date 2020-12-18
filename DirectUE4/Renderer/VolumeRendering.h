#pragma once

#include "Shader.h"
#include "ShaderParameters.h"
#include "GlobalShader.h"
#include "ScreenRendering.h"

/** Represents a subregion of a volume texture. */
struct FVolumeBounds
{
	int32 MinX, MinY, MinZ;
	int32 MaxX, MaxY, MaxZ;

	FVolumeBounds() :
		MinX(0),
		MinY(0),
		MinZ(0),
		MaxX(0),
		MaxY(0),
		MaxZ(0)
	{}

	FVolumeBounds(int32 Max) :
		MinX(0),
		MinY(0),
		MinZ(0),
		MaxX(Max),
		MaxY(Max),
		MaxZ(Max)
	{}

	bool IsValid() const
	{
		return MaxX > MinX && MaxY > MinY && MaxZ > MinZ;
	}
};
/** Vertex shader used to write to a range of slices of a 3d volume texture. */
class FWriteToSliceVS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FWriteToSliceVS, Global, );
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;// IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		//OutEnvironment.CompilerFlags.Add(CFLAG_VertexToGeometryShader);
	}

	FWriteToSliceVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		UVScaleBias.Bind(Initializer.ParameterMap, ("UVScaleBias"));
		MinZ.Bind(Initializer.ParameterMap, ("MinZ"));
	}

	FWriteToSliceVS() {}

	void SetParameters(const FVolumeBounds& VolumeBounds, FIntVector VolumeResolution)
	{
		const float InvVolumeResolutionX = 1.0f / VolumeResolution.X;
		const float InvVolumeResolutionY = 1.0f / VolumeResolution.Y;
		SetShaderValue(GetVertexShader(), UVScaleBias, Vector4(
			(VolumeBounds.MaxX - VolumeBounds.MinX) * InvVolumeResolutionX,
			(VolumeBounds.MaxY - VolumeBounds.MinY) * InvVolumeResolutionY,
			VolumeBounds.MinX * InvVolumeResolutionX,
			VolumeBounds.MinY * InvVolumeResolutionY));
		SetShaderValue(GetVertexShader(), MinZ, VolumeBounds.MinZ);
	}

private:
	FShaderParameter UVScaleBias;
	FShaderParameter MinZ;
};

class FWriteToSliceGS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FWriteToSliceGS, Global, );
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;// IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4) && RHISupportsGeometryShaders(Parameters.Platform);
	}

	FWriteToSliceGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		MinZ.Bind(Initializer.ParameterMap, ("MinZ"));
	}
	FWriteToSliceGS() {}

	void SetParameters(int32 MinZValue)
	{
		SetShaderValue(GetGeometryShader(), MinZ, MinZValue);
	}

private:
	FShaderParameter MinZ;
};

extern void RasterizeToVolumeTexture(FVolumeBounds VolumeBounds);

/** Vertex buffer used for rendering into a volume texture. */
class FVolumeRasterizeVertexBuffer// : public FVertexBuffer
{
public:

	virtual void InitRHI()
	{
		// Used as a non-indexed triangle strip, so 4 vertices per quad
		const uint32 Size = 4 * sizeof(FScreenVertex);
		//FRHIResourceCreateInfo CreateInfo;
		//void* Buffer = nullptr;
		//VertexBufferRHI = RHICreateAndLockVertexBuffer(Size, BUF_Static, CreateInfo, Buffer);
		VertexBufferRHI = RHICreateVertexBuffer(Size, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, NULL);
		FScreenVertex DestVertex[4] ;// (FScreenVertex*)Buffer;

		// Setup a full - render target quad
		// A viewport and UVScaleBias will be used to implement rendering to a sub region
		DestVertex[0].Position = Vector2(1, -GProjectionSignY);
		DestVertex[0].UV = Vector2(1, 1);
		DestVertex[1].Position = Vector2(1, GProjectionSignY);
		DestVertex[1].UV = Vector2(1, 0);
		DestVertex[2].Position = Vector2(-1, -GProjectionSignY);
		DestVertex[2].UV = Vector2(0, 1);
		DestVertex[3].Position = Vector2(-1, GProjectionSignY);
		DestVertex[3].UV = Vector2(0, 0);

		D3D11DeviceContext->UpdateSubresource(VertexBufferRHI,0,NULL, DestVertex, Size,0);

		//RHIUnlockVertexBuffer(VertexBufferRHI);
	}

	ID3D11Buffer* VertexBufferRHI;
};

extern FVolumeRasterizeVertexBuffer GVolumeRasterizeVertexBuffer;