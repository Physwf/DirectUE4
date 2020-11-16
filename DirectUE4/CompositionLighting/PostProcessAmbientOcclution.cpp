#include "PostProcessAmbientOcclution.h"
#include "PostProcess/SceneFilterRendering.h"
#include "RenderTargets.h"
#include "UnrealMath.h"
#include "Scene.h"
#include "DeferredShading.h"
#include "SceneOcclusion.h"
#include "DirectXTex.h"
using namespace DirectX;

void RCPassPostProcessAmbientOcclusionSetup::Init()
{
	/*
	const D3D_SHADER_MACRO Macros[] = { { "INITIAL_PASS", IsInitialPass() ? "1" : "0" } };
	PSBytecode = CompilePixelShader(TEXT("./Shaders/PostProcessAmbientOcclusion.hlsl"), "MainSetupPS", Macros, sizeof(Macros)/ sizeof(D3D_SHADER_MACRO));
	GetShaderParameterAllocations(PSBytecode, PSParams);
	PS = CreatePixelShader(PSBytecode);

	RenderTargets& SceneContext = RenderTargets::Get();
	GBufferASRV = CreateShaderResourceView2D(SceneContext.GetGBufferATexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	GBufferASamplerState = TStaticSamplerState<>::GetRHI();
	GBufferBSRV = CreateShaderResourceView2D(SceneContext.GetGBufferBTexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	GBufferBSamplerState = TStaticSamplerState<>::GetRHI();
	SceneDepthSRV = CreateShaderResourceView2D(SceneContext.GetSceneDepthTexture(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1, 0);
	SceneDepthSamplerState = TStaticSamplerState<>::GetRHI();

	RasterState = TStaticRasterizerState<>::GetRHI();
	BlendState = TStaticBlendState<>::GetRHI();

	ID3D11Texture2D* Input = IsInitialPass() ? Inputs[0] : Inputs[1];
	D3D11_TEXTURE2D_DESC InputDesc;
	Input->GetDesc(&InputDesc);
	IntPoint InputExtent(InputDesc.Width,InputDesc.Height);
	OutputExtent = IntPoint::DivideAndRoundUp(InputExtent, 2);
	Output = CreateTexture2D(OutputExtent.X, OutputExtent.Y, DXGI_FORMAT_R32G32B32A32_FLOAT,true, true,false,1);
	OutputRTV = CreateRenderTargetView2D(Output, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
	*/
}

void RCPassPostProcessAmbientOcclusionSetup::Process(FViewInfo& View)
{
	/*
	D3D11DeviceContext->OMSetRenderTargets(1, &OutputRTV, NULL);

	D3D11DeviceContext->IASetInputLayout(GFilterInputLayout);
	UINT Strides = sizeof(FilterVertex);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0,1,&GScreenRectangleVertexBuffer,&Strides,&Offset);
	D3D11DeviceContext->IASetIndexBuffer(GScreenRectangleIndexBuffer,DXGI_FORMAT_R16_UINT,0);

	D3D11DeviceContext->VSSetShader(GCommonPostProcessVS, 0, 0);
	D3D11DeviceContext->PSSetShader(PS, 0, 0);

	const ParameterAllocation& ViewParam = PSParams["View"];
	const ParameterAllocation& GBufferATextureParam = PSParams["SceneTexturesStruct_GBufferATexture"];
	const ParameterAllocation& GBufferATextureSamplerParam = PSParams["SceneTexturesStruct_GBufferATextureSampler"];
// 	const ParameterAllocation& GBufferBTextureParam = PSParams["SceneTexturesStruct_GBufferBTexture"];
// 	const ParameterAllocation& GBufferBTextureSamplerParam = PSParams["SceneTexturesStruct_GBufferBTextureSampler"];
	const ParameterAllocation& SceneDepthTextureParam = PSParams["SceneTexturesStruct_SceneDepthTexture"];
	const ParameterAllocation& SceneDepthTextureSamplerParam = PSParams["SceneTexturesStruct_SceneDepthTextureSampler"];

	D3D11DeviceContext->PSSetConstantBuffers(ViewParam.BufferIndex, 1, &View.ViewUniformBuffer);
	D3D11DeviceContext->PSSetShaderResources(GBufferATextureParam.BaseIndex, GBufferATextureParam.Size, &GBufferASRV);
	D3D11DeviceContext->PSSetSamplers(GBufferATextureSamplerParam.BaseIndex, GBufferATextureSamplerParam.Size, &GBufferASamplerState);
// 	D3D11DeviceContext->PSSetShaderResources(GBufferBTextureParam.BaseIndex, GBufferBTextureParam.Size, &GBufferBSRV);
// 	D3D11DeviceContext->PSSetSamplers(GBufferBTextureSamplerParam.BaseIndex, GBufferBTextureSamplerParam.Size, &GBufferBSamplerState);
	D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SceneDepthSRV);
	D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &SceneDepthSamplerState);

	if (IsInitialPass())
	{
		const ParameterAllocation& PostprocessInput0SizeParam = PSParams["PostprocessInput0Size"];
		Vector4 PostprocessInput0Size = Vector4((float)OutputExtent.X, (float)OutputExtent.Y, 1.f / OutputExtent.X, 1.f / OutputExtent.Y);
		memcpy(GlobalConstantBufferData + PostprocessInput0SizeParam.BaseIndex, &PostprocessInput0Size, PostprocessInput0SizeParam.Size);
		D3D11DeviceContext->UpdateSubresource(GlobalConstantBuffer, 0, NULL, GlobalConstantBufferData, 0, 0);
		D3D11DeviceContext->PSSetConstantBuffers(PostprocessInput0SizeParam.BufferIndex, 1, &GlobalConstantBuffer);
	}
	else
	{
		const ParameterAllocation& PostprocessInput1SizeParam = PSParams["PostprocessInput1Size"];
		Vector4 PostprocessInput1Size = Vector4((float)OutputExtent.X, (float)OutputExtent.Y, 1.f / (float)OutputExtent.X, 1.f / (float)OutputExtent.Y);
		memcpy(GlobalConstantBufferData + PostprocessInput1SizeParam.BaseIndex, &PostprocessInput1Size, PostprocessInput1SizeParam.Size);
		D3D11DeviceContext->UpdateSubresource(GlobalConstantBuffer, 0, NULL, GlobalConstantBufferData, 0, 0);
		D3D11DeviceContext->PSSetConstantBuffers(PostprocessInput1SizeParam.BufferIndex, 1, &GlobalConstantBuffer);
	}

	D3D11DeviceContext->RSSetState(RasterState);

	uint32 ScaleFactor = RenderTargets::Get().GetBufferSizeXY().X / OutputExtent.X;
	D3D11_VIEWPORT Viewport;
	Viewport.TopLeftX = GViewport.TopLeftX / ScaleFactor;
	Viewport.TopLeftY = GViewport.TopLeftY / ScaleFactor;
	Viewport.Width = GViewport.Width / ScaleFactor;
	Viewport.Height = GViewport.Height / ScaleFactor;
	Viewport.MinDepth = 0.f;
	Viewport.MaxDepth = 1.0f;
	D3D11DeviceContext->RSSetViewports(1, &Viewport);
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);

	D3D11DeviceContext->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* SRV = NULL;
	ID3D11SamplerState* Sampler = NULL;
	D3D11DeviceContext->PSSetShaderResources(GBufferATextureParam.BaseIndex, GBufferATextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(GBufferATextureSamplerParam.BaseIndex, GBufferATextureSamplerParam.Size, &Sampler);
	D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &Sampler);
	*/
}

bool RCPassPostProcessAmbientOcclusionSetup::IsInitialPass() const
{
// 	if (!Inputs[0] && Inputs[1])
// 	{
// 		return false;
// 	}
// 	if (Inputs[0] && !Inputs[1])
// 	{
// 		return true;
// 	}
	return false;
}

void RCPassPostProcessAmbientOcclusion::Init(bool InAOSetupAsInput)
{
	/*
	bAOSetupAsInput = InAOSetupAsInput;
	bool bDoUpsample = (Inputs[2] != 0);
	const D3D_SHADER_MACRO Macros[] = { { "COMPUTE_SHADER",  "0" } , {"SHADER_QUALITY", "2" }, {"USE_UPSAMPLE", bDoUpsample ? "1" : "0" },  { "USE_AO_SETUP_AS_INPUT" , bAOSetupAsInput ? "1" : "0" } };
	PSBytecode = CompilePixelShader(TEXT("./Shaders/PostProcessAmbientOcclusion.hlsl"), "MainPS", Macros, sizeof(Macros) / sizeof(D3D_SHADER_MACRO));
	GetShaderParameterAllocations(PSBytecode, PSParams);
	PS = CreatePixelShader(PSBytecode);
	RasterState = TStaticRasterizerState<>::GetRHI();
	BlendState = TStaticBlendState<>::GetRHI();

	if (bAOSetupAsInput)
	{
		D3D11_TEXTURE2D_DESC InputDesc;
		Inputs[0]->GetDesc(&InputDesc);
		OutputExtent = IntPoint(InputDesc.Width, InputDesc.Height);
		Output = CreateTexture2D(OutputExtent.X, OutputExtent.Y, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false, 1);
		OutputRTV = CreateRenderTargetView2D(Output, DXGI_FORMAT_R8G8B8A8_UNORM,0);
	}
	else
	{
		RenderTargets& SceneContext = RenderTargets::Get();
		Output = SceneContext.GetScreenSpaceAO();
		D3D11_TEXTURE2D_DESC Desc;
		Output->GetDesc(&Desc);
		OutputExtent = IntPoint(Desc.Width, Desc.Height);
		OutputRTV = CreateRenderTargetView2D(Output, DXGI_FORMAT_R8_UNORM, 0);
	}
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromTGAFile(TEXT("./dx11demo/RandomNormalTexture.tga"), &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), Metadata.mipLevels, Metadata, &RandomNormalSRV);
			RandomNormalSampler = TStaticSamplerState<>::GetRHI();
		}
	}
	RenderTargets& SceneContext = RenderTargets::Get();
	GBufferASRV = CreateShaderResourceView2D(SceneContext.GetGBufferATexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	GBufferASamplerState = TStaticSamplerState<>::GetRHI();
	GBufferBSRV = CreateShaderResourceView2D(SceneContext.GetGBufferBTexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	GBufferBSamplerState = TStaticSamplerState<>::GetRHI();
	SceneDepthSRV = CreateShaderResourceView2D(SceneContext.GetSceneDepthTexture(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1, 0);
	SceneDepthSamplerState = TStaticSamplerState<>::GetRHI();
	PostprocessInput0 = GBufferASRV;
	PostprocessInput0Sampler = GBufferASamplerState;
	PostprocessInput1 = GBufferASRV;
	PostprocessInput1Sampler = GBufferASamplerState;
	PostprocessInput2 = GBufferASRV;
	PostprocessInput2Sampler = GBufferASamplerState;
// 	{
// 		TexMetadata Metadata;
// 		ScratchImage sImage;
// 		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/BlackDummy.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
// 		{
// 			CreateShaderResourceView(D3D11Device, sImage.GetImages(), Metadata.mipLevels, Metadata, &PostprocessInput3);
// 			PostprocessInput3Sampler = TStaticSamplerState<>::GetRHI();
// 		}
// 	}
	PostprocessInput3 = HZBSRV;
	PostprocessInput3Sampler = TStaticSamplerState<>::GetRHI();
	*/
}

void RCPassPostProcessAmbientOcclusion::Process(FViewInfo& View)
{
	/*
	ID3D11RenderTargetView* DestRenderTarget = NULL;
	RenderTargets& SceneContext = RenderTargets::Get();

	DestRenderTarget = OutputRTV;

	IntPoint TexSize = Inputs[0] ? OutputExtent : SceneContext.GetBufferSizeXY();

	uint32 ScaleToFullRes = SceneContext.GetBufferSizeXY().X / TexSize.X;

	IntRect ViewRect = IntRect::DivideAndRoundUp(View.ViewRect, ScaleToFullRes);

	const int32 ShaderQuality = 2;

	bool bDoUpsample = (Inputs[2] != 0);

	const ParameterAllocation& ViewParam = PSParams["View"];
	D3D11DeviceContext->PSSetConstantBuffers(ViewParam.BufferIndex, 1, &View.ViewUniformBuffer);
	ProcessPS(DestRenderTarget, View, ViewRect, TexSize, ShaderQuality, bDoUpsample);
	*/
}

Vector4 GetHZBValue(const FViewInfo& View)
{
	const Vector2 HZBScaleFactor(
		float(View.ViewRect.Width()) / float(2 * View.HZBMipmap0Size.X),
		float(View.ViewRect.Height()) / float(2 * View.HZBMipmap0Size.Y));

	// from -1..1 to UV 0..1*HZBScaleFactor
	// .xy:mul, zw:add
	const Vector4 HZBRemappingValue(
		0.5f * HZBScaleFactor.X,
		-0.5f * HZBScaleFactor.Y,
		0.5f * HZBScaleFactor.X,
		0.5f * HZBScaleFactor.Y);

	return HZBRemappingValue;
}

void RCPassPostProcessAmbientOcclusion::ProcessPS(ID3D11RenderTargetView* DestRenderTarget, FViewInfo& View, const IntRect& ViewRect, const FIntPoint& TexSize, int32 ShaderQuality, bool bDoUpsample)
{
	/*
	D3D11DeviceContext->OMSetRenderTargets(1, &DestRenderTarget, NULL);

	D3D11DeviceContext->IASetInputLayout(GFilterInputLayout);
	UINT Strides = sizeof(FilterVertex);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &GScreenRectangleVertexBuffer, &Strides, &Offset);
	D3D11DeviceContext->IASetIndexBuffer(GScreenRectangleIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	D3D11DeviceContext->VSSetShader(GCommonPostProcessVS, 0, 0);
	D3D11DeviceContext->PSSetShader(PS, 0, 0);
	
	const ParameterAllocation& HZBRemappingParam = PSParams.at("HZBRemapping");
	if (bAOSetupAsInput)
	{
		const ParameterAllocation& PostprocessInput0Param = PSParams.at("PostprocessInput0");
		const ParameterAllocation& PostprocessInput0SamplerParam = PSParams.at("PostprocessInput0Sampler");
		D3D11DeviceContext->PSSetShaderResources(PostprocessInput0Param.BaseIndex, PostprocessInput0Param.Size, &PostprocessInput0);
		D3D11DeviceContext->PSSetSamplers(PostprocessInput0SamplerParam.BaseIndex, PostprocessInput0SamplerParam.Size, &PostprocessInput0Sampler);
	}
	const ParameterAllocation& PostprocessInput1Param = PSParams.at("PostprocessInput1");
	const ParameterAllocation& PostprocessInput1SamplerParam = PSParams.at("PostprocessInput1Sampler");
	const ParameterAllocation& PostprocessInput2Param = PSParams.at("PostprocessInput2");
	const ParameterAllocation& PostprocessInput2SamplerParam = PSParams.at("PostprocessInput2Sampler");
	const ParameterAllocation& PostprocessInput2SizeParam = PSParams.at("PostprocessInput2Size");
	const ParameterAllocation& PostprocessInput3Param = PSParams.at("PostprocessInput3");
	const ParameterAllocation& PostprocessInput3SamplerParam = PSParams.at("PostprocessInput3Sampler");
	const ParameterAllocation& RandomNormalTextureParam = PSParams.at("RandomNormalTexture");
	const ParameterAllocation& RandomNormalTextureSamplerParam = PSParams.at("RandomNormalTextureSampler");

	

	const ParameterAllocation& ScreenSpaceAOParamsParam = PSParams.at("ScreenSpaceAOParams");

	
	D3D11DeviceContext->PSSetShaderResources(PostprocessInput1Param.BaseIndex, PostprocessInput1Param.Size, &PostprocessInput1);
	D3D11DeviceContext->PSSetSamplers(PostprocessInput1SamplerParam.BaseIndex, PostprocessInput1SamplerParam.Size, &PostprocessInput1Sampler);
	D3D11DeviceContext->PSSetShaderResources(PostprocessInput2Param.BaseIndex, PostprocessInput2Param.Size, &PostprocessInput2);
	D3D11DeviceContext->PSSetSamplers(PostprocessInput2SamplerParam.BaseIndex, PostprocessInput2SamplerParam.Size, &PostprocessInput2Sampler);
	D3D11DeviceContext->PSSetShaderResources(PostprocessInput3Param.BaseIndex, PostprocessInput3Param.Size, &PostprocessInput3);
	D3D11DeviceContext->PSSetSamplers(PostprocessInput3SamplerParam.BaseIndex, PostprocessInput3SamplerParam.Size, &PostprocessInput3Sampler);
	D3D11DeviceContext->PSSetShaderResources(RandomNormalTextureParam.BaseIndex, RandomNormalTextureParam.Size, &RandomNormalSRV);
	D3D11DeviceContext->PSSetSamplers(RandomNormalTextureSamplerParam.BaseIndex, RandomNormalTextureSamplerParam.Size, &RandomNormalSampler);

	if (bDoUpsample)
	{
		const ParameterAllocation& GBufferATextureParam = PSParams.at("SceneTexturesStruct_GBufferATexture");
		const ParameterAllocation& GBufferATextureSamplerParam = PSParams.at("SceneTexturesStruct_GBufferATextureSampler");
		const ParameterAllocation& GBufferBTextureParam = PSParams.at("SceneTexturesStruct_GBufferBTexture");
		const ParameterAllocation& GBufferBTextureSamplerParam = PSParams.at("SceneTexturesStruct_GBufferBTextureSampler");
		const ParameterAllocation& SceneDepthTextureParam = PSParams.at("SceneTexturesStruct_SceneDepthTexture");
		const ParameterAllocation& SceneDepthTextureSamplerParam = PSParams.at("SceneTexturesStruct_SceneDepthTextureSampler");
		D3D11DeviceContext->PSSetShaderResources(GBufferATextureParam.BaseIndex, GBufferATextureParam.Size, &GBufferASRV);
		D3D11DeviceContext->PSSetSamplers(GBufferATextureSamplerParam.BaseIndex, GBufferATextureSamplerParam.Size, &GBufferASamplerState);
		D3D11DeviceContext->PSSetShaderResources(GBufferBTextureParam.BaseIndex, GBufferBTextureParam.Size, &GBufferBSRV);
		D3D11DeviceContext->PSSetSamplers(GBufferBTextureSamplerParam.BaseIndex, GBufferBTextureSamplerParam.Size, &GBufferBSamplerState);
		D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SceneDepthSRV);
		D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &SceneDepthSamplerState);
	}

	Vector4 HZBRemapping = GetHZBValue(View);
	memcpy(GlobalConstantBufferData + HZBRemappingParam.BaseIndex, &HZBRemapping, HZBRemappingParam.Size);
	Vector4 PostprocessInput2Size;
	memcpy(GlobalConstantBufferData + PostprocessInput2SizeParam.BaseIndex, &PostprocessInput2Size, PostprocessInput2SizeParam.Size);
	Vector4 ScreenSpaceAOParams[6]
		= {
		Vector4(2.00f, 0.003f, 0.0125f, 0.50f),
		Vector4(10.4375f, 6.0625f, 0.2125f, 1.72461f),
		Vector4(2.00f, 0.005f, 0.00f, 0.60f),
		Vector4(0.00f, 0.00f, 1.00f, 1.00f),
		Vector4(0.0002f, -0.60f, 0.40f, 0.00f),
		Vector4((float)ViewRect.Width(), (float)ViewRect.Height(), (float)ViewRect.Min.X, (float)ViewRect.Min.Y),
	};
// 	Value[0] = FVector4(Settings.AmbientOcclusionPower, Settings.AmbientOcclusionBias / 1000.0f, 1.0f / Settings.AmbientOcclusionDistance_DEPRECATED, Settings.AmbientOcclusionIntensity);
// 	Value[1] = FVector4(ViewportUVToRandomUV.X, ViewportUVToRandomUV.Y, AORadiusInShader, Ratio);
// 	Value[2] = FVector4(ScaleToFullRes, Settings.AmbientOcclusionMipThreshold / ScaleToFullRes, ScaleRadiusInWorldSpace, Settings.AmbientOcclusionMipBlend);
// 	Value[3] = FVector4(TemporalOffset.X, TemporalOffset.Y, StaticFraction, InvTanHalfFov);
// 	Value[4] = FVector4(InvFadeRadius, -(Settings.AmbientOcclusionFadeDistance - FadeRadius) * InvFadeRadius, HzbStepMipLevelFactorValue, 0);
// 	Value[5] = FVector4(View.ViewRect.Width(), View.ViewRect.Height(), ViewRect.Min.X, ViewRect.Min.Y);
	memcpy(GlobalConstantBufferData + ScreenSpaceAOParamsParam.BaseIndex, &ScreenSpaceAOParams, ScreenSpaceAOParamsParam.Size);

	D3D11DeviceContext->UpdateSubresource(GlobalConstantBuffer, 0, 0, &GlobalConstantBufferData, 0, 0);
	D3D11DeviceContext->PSSetConstantBuffers(HZBRemappingParam.BufferIndex, 1, &GlobalConstantBuffer);

	D3D11DeviceContext->RSSetState(RasterState);
	D3D11_VIEWPORT Viewport;
	Viewport.TopLeftX = Viewport.TopLeftY = 0;
	Viewport.Width = (float)ViewRect.Max.X - (float)ViewRect.Min.X;
	Viewport.Height = (float)ViewRect.Max.Y - (float)ViewRect.Min.Y;
	Viewport.MinDepth = 0;
	Viewport.MaxDepth = 1;
	D3D11DeviceContext->RSSetViewports(1, &Viewport);
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);

	D3D11DeviceContext->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* SRV = NULL;
	ID3D11SamplerState* Sampler = NULL;
	D3D11DeviceContext->PSSetShaderResources(PostprocessInput1Param.BaseIndex, PostprocessInput1Param.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(PostprocessInput1SamplerParam.BaseIndex, PostprocessInput1SamplerParam.Size, &Sampler);
	D3D11DeviceContext->PSSetShaderResources(PostprocessInput2Param.BaseIndex, PostprocessInput2Param.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(PostprocessInput2SamplerParam.BaseIndex, PostprocessInput2SamplerParam.Size, &Sampler);
	D3D11DeviceContext->PSSetShaderResources(PostprocessInput3Param.BaseIndex, PostprocessInput3Param.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(PostprocessInput3SamplerParam.BaseIndex, PostprocessInput3SamplerParam.Size, &Sampler);
	D3D11DeviceContext->PSSetShaderResources(RandomNormalTextureParam.BaseIndex, RandomNormalTextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(RandomNormalTextureSamplerParam.BaseIndex, RandomNormalTextureSamplerParam.Size, &Sampler);

	if (bDoUpsample)
	{
		const ParameterAllocation& GBufferATextureParam = PSParams.at("SceneTexturesStruct_GBufferATexture");
		const ParameterAllocation& GBufferATextureSamplerParam = PSParams.at("SceneTexturesStruct_GBufferATextureSampler");
		const ParameterAllocation& GBufferBTextureParam = PSParams.at("SceneTexturesStruct_GBufferBTexture");
		const ParameterAllocation& GBufferBTextureSamplerParam = PSParams.at("SceneTexturesStruct_GBufferBTextureSampler");
		const ParameterAllocation& SceneDepthTextureParam = PSParams.at("SceneTexturesStruct_SceneDepthTexture");
		const ParameterAllocation& SceneDepthTextureSamplerParam = PSParams.at("SceneTexturesStruct_SceneDepthTextureSampler");

		D3D11DeviceContext->PSSetShaderResources(GBufferATextureParam.BaseIndex, GBufferATextureParam.Size, &SRV);
		D3D11DeviceContext->PSSetSamplers(GBufferATextureSamplerParam.BaseIndex, GBufferATextureSamplerParam.Size, &Sampler);
		D3D11DeviceContext->PSSetShaderResources(GBufferBTextureParam.BaseIndex, GBufferBTextureParam.Size, &SRV);
		D3D11DeviceContext->PSSetSamplers(GBufferBTextureSamplerParam.BaseIndex, GBufferBTextureSamplerParam.Size, &Sampler);
		D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SRV);
		D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &Sampler);
	}

	if (bAOSetupAsInput)
	{
		const ParameterAllocation& PostprocessInput0Param = PSParams.at("PostprocessInput0");
		const ParameterAllocation& PostprocessInput0SamplerParam = PSParams.at("PostprocessInput0Sampler");
		D3D11DeviceContext->PSSetShaderResources(PostprocessInput0Param.BaseIndex, PostprocessInput0Param.Size, &SRV);
		D3D11DeviceContext->PSSetSamplers(PostprocessInput0SamplerParam.BaseIndex, PostprocessInput0SamplerParam.Size, &Sampler);
	}
	*/
}

void RCPassPostProcessBasePassAO::Init()
{
	/*
	PSBytecode = CompilePixelShader(TEXT("./Shaders/PostProcessAmbientOcclusion.hlsl"), "BasePassAOPS");
	GetShaderParameterAllocations(PSBytecode, PSParams);
	PS = CreatePixelShader(PSBytecode);
	
	RenderTargets& SceneContext = RenderTargets::Get();
	GBufferBSRV = CreateShaderResourceView2D(SceneContext.GetGBufferBTexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	GBufferBSamplerState = TStaticSamplerState<>::GetRHI();
	GBufferCSRV = CreateShaderResourceView2D(SceneContext.GetGBufferCTexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	GBufferCSamplerState = TStaticSamplerState<>::GetRHI();
	ScreenSpaceAOSRV = CreateShaderResourceView2D(SceneContext.GetScreenSpaceAO(), DXGI_FORMAT_R8_UNORM, 1, 0);
	ScreenSpaceAOState = TStaticSamplerState<>::GetRHI();
	RasterState = TStaticRasterizerState<>::GetRHI();
	DepthStencilState = TStaticDepthStencilState<FALSE, D3D11_COMPARISON_ALWAYS>::GetRHI();
	//GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_DestColor, BF_Zero, BO_Add, BF_DestAlpha, BF_Zero>::GetRHI();
	BlendState = TStaticBlendState<FALSE,FALSE,0x07+0x08, D3D11_BLEND_OP_ADD, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_ZERO>::GetRHI();
	*/
}

void RCPassPostProcessBasePassAO::Process(FViewInfo& View)
{
	/*
	RenderTargets& SceneContext = RenderTargets::Get();
	SceneContext.BeginRenderingSceneColor();

	D3D11DeviceContext->IASetInputLayout(GFilterInputLayout);
	UINT Strides = sizeof(FilterVertex);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &GScreenRectangleVertexBuffer, &Strides, &Offset);
	D3D11DeviceContext->IASetIndexBuffer(GScreenRectangleIndexBuffer, DXGI_FORMAT_R16_UINT,0);

	D3D11DeviceContext->VSSetShader(GCommonPostProcessVS, 0, 0);
	D3D11DeviceContext->PSSetShader(PS, 0, 0);

	const ParameterAllocation& GBufferBTextureParam = PSParams.at("SceneTexturesStruct_GBufferBTexture");
	const ParameterAllocation& GBufferBTextureSamplerParam = PSParams.at("SceneTexturesStruct_GBufferBTextureSampler");
	const ParameterAllocation& GBufferCTextureParam = PSParams.at("SceneTexturesStruct_GBufferCTexture");
	const ParameterAllocation& GBufferCTextureSamplerParam = PSParams.at("SceneTexturesStruct_GBufferCTextureSampler");
	const ParameterAllocation& GBufferAOTextureParam = PSParams.at("SceneTexturesStruct_ScreenSpaceAOTexture");
	const ParameterAllocation& GBufferAOTextureSamplerParam = PSParams.at("SceneTexturesStruct_ScreenSpaceAOTextureSampler");

	const ParameterAllocation& ScreenSpaceAOParamsParam = PSParams.at("ScreenSpaceAOParams");

	D3D11DeviceContext->PSSetShaderResources(GBufferBTextureParam.BaseIndex, GBufferBTextureParam.Size, &GBufferBSRV);
	D3D11DeviceContext->PSSetSamplers(GBufferBTextureSamplerParam.BaseIndex, GBufferBTextureSamplerParam.Size, &GBufferBSamplerState);
	D3D11DeviceContext->PSSetShaderResources(GBufferCTextureParam.BaseIndex, GBufferCTextureParam.Size, &GBufferBSRV);
	D3D11DeviceContext->PSSetSamplers(GBufferCTextureSamplerParam.BaseIndex, GBufferCTextureSamplerParam.Size, &GBufferCSamplerState);
	D3D11DeviceContext->PSSetShaderResources(GBufferAOTextureParam.BaseIndex, GBufferAOTextureParam.Size, &ScreenSpaceAOSRV);
	D3D11DeviceContext->PSSetSamplers(GBufferAOTextureSamplerParam.BaseIndex, GBufferAOTextureSamplerParam.Size, &ScreenSpaceAOState);

	Vector4 ScreenSpaceAOParams[6]
		= {
		Vector4(2.00f, 0.003f, 0.0125f, 0.50f),
		Vector4(10.4375f, 6.0625f, 0.2125f, 1.72461f),
		Vector4(2.00f, 0.005f, 0.00f, 0.60f),
		Vector4(0.00f, 0.00f, 1.00f, 1.00f),
		Vector4(0.0002f, -0.60f, 0.40f, 0.00f),
		Vector4((float)View.ViewRect.Width(), (float)View.ViewRect.Height(), (float)View.ViewRect.Min.X, (float)View.ViewRect.Min.Y),
	};
	
	memcpy(GlobalConstantBufferData + ScreenSpaceAOParamsParam.BaseIndex, &ScreenSpaceAOParams, ScreenSpaceAOParamsParam.Size);
	D3D11DeviceContext->PSSetConstantBuffers(ScreenSpaceAOParamsParam.BufferIndex, 1, &GlobalConstantBuffer);

	D3D11DeviceContext->RSSetState(RasterState);
	D3D11_VIEWPORT Viewport;
	Viewport.TopLeftX = Viewport.TopLeftY = 0;
	Viewport.Width = (float)View.ViewRect.Max.X - (float)View.ViewRect.Min.X;
	Viewport.Height = (float)View.ViewRect.Max.Y - (float)View.ViewRect.Min.Y;
	Viewport.MinDepth = 0;
	Viewport.MaxDepth = 1;
	D3D11DeviceContext->RSSetViewports(1, &Viewport);
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

	D3D11DeviceContext->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* SRV = NULL;
	ID3D11SamplerState* Sampler = NULL;
	D3D11DeviceContext->PSSetShaderResources(GBufferBTextureParam.BaseIndex, GBufferBTextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(GBufferBTextureSamplerParam.BaseIndex, GBufferBTextureSamplerParam.Size, &Sampler);
	D3D11DeviceContext->PSSetShaderResources(GBufferCTextureParam.BaseIndex, GBufferCTextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(GBufferCTextureSamplerParam.BaseIndex, GBufferCTextureSamplerParam.Size, &Sampler);
	D3D11DeviceContext->PSSetShaderResources(GBufferAOTextureParam.BaseIndex, GBufferAOTextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(GBufferAOTextureSamplerParam.BaseIndex, GBufferAOTextureSamplerParam.Size, &Sampler);
	*/
}
