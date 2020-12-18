#pragma once

#include "D3D11RHI.h"
#include "ShaderParameters.h"

class FShaderParameterMap;
struct FRenderingCompositePassContext;

// This is the index for the texture input of this pass.
// More that that should not be needed.
// Could be an uint32 but for better readability and type safety it's an enum.
// Counting starts from 0 in consecutive order.
enum EPassInputId
{
	ePId_Input0,
	ePId_Input1,
	ePId_Input2,
	ePId_Input3,
	ePId_Input4,
	ePId_Input5,
	ePId_Input6,
	ePId_Input7,
	ePId_Input8,
	ePId_Input9,
	ePId_Input10,
	/*	ePId_Input11,
	ePId_Input12,
	ePId_Input13,
	ePId_Input14,
	ePId_Input15*/

	// to get the total count of inputs
	ePId_Input_MAX
};

// Usually the same as the MRT number but it doesn't have to be implemented as MRT.
// More that that should not be needed.
// Could be an uint32 but for better readability and type safety it's an enum.
// Counting starts from 0 in consecutive order.
enum EPassOutputId
{
	ePId_Output0,
	ePId_Output1,
	ePId_Output2,
	ePId_Output3,
	ePId_Output4,
	ePId_Output5,
	ePId_Output6,
	ePId_Output7
};

enum EFallbackColor
{
	// float4(0,0,0,0) BlackDummy
	eFC_0000,
	// float4(1,1,1,1) WhiteDummy 
	eFC_1111,
	// float4(0,0,0,1) BlackAlphaOneDummy
	eFC_0001,
};

// currently hard coded to 4 input textures
// convenient but not the most optimized solution
struct FPostProcessPassParameters
{
	/** Initialization constructor. */
	void Bind(const FShaderParameterMap& ParameterMap);

	/** Set the pixel shader parameter values. */
	void SetPS(ID3D11PixelShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter = TStaticSamplerState<>::GetRHI(), EFallbackColor FallbackColor = eFC_0000, ID3D11SamplerState** FilterOverrideArray = 0);

	/** Set the compute shader parameter values. */
	void SetCS(ID3D11ComputeShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter = TStaticSamplerState<>::GetRHI(), EFallbackColor FallbackColor = eFC_0000, ID3D11SamplerState** FilterOverrideArray = 0);

	/** Set the vertex shader parameter values. */
	void SetVS(ID3D11VertexShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter = TStaticSamplerState<>::GetRHI(), EFallbackColor FallbackColor = eFC_0000, ID3D11SamplerState** FilterOverrideArray = 0);


private:

	FShaderParameter ViewportSize;
	FShaderParameter ViewportRect;
	FShaderResourceParameter PostprocessInputParameter[ePId_Input_MAX];
	FShaderResourceParameter PostprocessInputParameterSampler[ePId_Input_MAX];
	FShaderParameter PostprocessInputSizeParameter[ePId_Input_MAX];
	FShaderParameter PostProcessInputMinMaxParameter[ePId_Input_MAX];
	FShaderParameter ScreenPosToPixel;
	FShaderParameter SceneColorBufferUVViewport;
	FShaderResourceParameter BilinearTextureSampler0;
	FShaderResourceParameter BilinearTextureSampler1;

public:
	// @param Filter can be 0 if FilterOverrideArray is used
	// @param FilterOverrideArray can be 0 if Filter is used
	template< typename TShaderRHIParamRef>
	void Set(
		const TShaderRHIParamRef& ShaderRHI,
		const FRenderingCompositePassContext& Context,
		ID3D11SamplerState* Filter,
		EFallbackColor FallbackColor,
		ID3D11SamplerState** FilterOverrideArray = 0
	);
};

