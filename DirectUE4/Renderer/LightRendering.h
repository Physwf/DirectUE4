#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "SceneView.h"
#include "SceneManagement.h"
#include "Shader.h"
#include "LightSceneInfo.h"

struct alignas(16) FDeferredLightUniformStruct
{
	FDeferredLightUniformStruct()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		FVector LightPosition;
		float LightInvRadius;
		FVector LightColor;
		float LightFalloffExponent;
		FVector NormalizedLightDirection;
		FVector NormalizedLightTangent;
		Vector2 SpotAngles;
		float SpecularScale;
		float SourceRadius;
		float SoftSourceRadius;
		float SourceLength;
		float ContactShadowLength;
		Vector2 DistanceFadeMAD;
		Vector4 ShadowMapChannelMask;
		uint32 ShadowedBits;
		uint32 LightingChannelMask;
		float VolumetricScatteringIntensity;
	} Constants;

	ID3D11ShaderResourceView*  SourceTexture;// Texture2D

	static std::string GetConstantBufferName()
	{
		return "DeferredLightUniforms";
	}
#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FDeferredLightUniformStruct& DeferredLightUniforms)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(DeferredLightUniforms, SourceTexture);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FDeferredLightUniformStruct& DeferredLightUniforms)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FDeferredLightUniformStruct& DeferredLightUniforms)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

extern float GetLightFadeFactor(const FSceneView& View, const FLightSceneProxy* Proxy);

template<typename ShaderRHIParamRef>
void SetDeferredLightParameters(
	const ShaderRHIParamRef ShaderRHI,
	const TShaderUniformBufferParameter<FDeferredLightUniformStruct>& DeferredLightUniformBufferParameter,
	const FLightSceneInfo* LightSceneInfo,
	const FSceneView& View)
{
	FDeferredLightUniformStruct DeferredLightUniformsValue;

	FLightParameters LightParameters;

	LightSceneInfo->Proxy->GetParameters(LightParameters);

	DeferredLightUniformsValue.Constants.LightPosition = LightParameters.LightPositionAndInvRadius;
	DeferredLightUniformsValue.Constants.LightInvRadius = LightParameters.LightPositionAndInvRadius.W;
	DeferredLightUniformsValue.Constants.LightColor = LightParameters.LightColorAndFalloffExponent;
	DeferredLightUniformsValue.Constants.LightFalloffExponent = LightParameters.LightColorAndFalloffExponent.W;
	DeferredLightUniformsValue.Constants.NormalizedLightDirection = LightParameters.NormalizedLightDirection;
	DeferredLightUniformsValue.Constants.NormalizedLightTangent = LightParameters.NormalizedLightTangent;
	DeferredLightUniformsValue.Constants.SpotAngles = LightParameters.SpotAngles;
	DeferredLightUniformsValue.Constants.SpecularScale = LightParameters.SpecularScale;
	DeferredLightUniformsValue.Constants.SourceRadius = LightParameters.LightSourceRadius;
	DeferredLightUniformsValue.Constants.SoftSourceRadius = LightParameters.LightSoftSourceRadius;
	DeferredLightUniformsValue.Constants.SourceLength = LightParameters.LightSourceLength;
	DeferredLightUniformsValue.SourceTexture = LightParameters.SourceTexture;

	const Vector2 FadeParams = LightSceneInfo->Proxy->GetDirectionalLightDistanceFadeParameters(LightSceneInfo->IsPrecomputedLightingValid(), View.MaxShadowCascades);

	// use MAD for efficiency in the shader
	DeferredLightUniformsValue.Constants.DistanceFadeMAD = Vector2(FadeParams.Y, -FadeParams.X * FadeParams.Y);

	int32 ShadowMapChannel = LightSceneInfo->Proxy->GetShadowMapChannel();

	//static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));
	const bool bAllowStaticLighting = true;// (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnRenderThread() != 0);

	if (!bAllowStaticLighting)
	{
		ShadowMapChannel = INDEX_NONE;
	}

	DeferredLightUniformsValue.Constants.ShadowMapChannelMask = Vector4(
		ShadowMapChannel == 0 ? 1.f : 0,
		ShadowMapChannel == 1 ? 1.f : 0,
		ShadowMapChannel == 2 ? 1.f : 0,
		ShadowMapChannel == 3 ? 1.f : 0);

	const bool bDynamicShadows = true;// View.Family->EngineShowFlags.DynamicShadows && GetShadowQuality() > 0;
	const bool bHasLightFunction = false;// LightSceneInfo->Proxy->GetLightFunctionMaterial() != NULL;
	DeferredLightUniformsValue.Constants.ShadowedBits = LightSceneInfo->Proxy->CastsStaticShadow() || bHasLightFunction ? 1 : 0;
	DeferredLightUniformsValue.Constants.ShadowedBits |= LightSceneInfo->Proxy->CastsDynamicShadow() /*&& View.Family->EngineShowFlags.DynamicShadows*/ ? 3 : 0;

	DeferredLightUniformsValue.Constants.VolumetricScatteringIntensity = LightSceneInfo->Proxy->GetVolumetricScatteringIntensity();

	//static auto* ContactShadowsCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.ContactShadows"));
	DeferredLightUniformsValue.Constants.ContactShadowLength = 0;

	if (true/*ContactShadowsCVar && ContactShadowsCVar->GetValueOnRenderThread() != 0 && View.Family->EngineShowFlags.ContactShadows*/)
	{
		DeferredLightUniformsValue.Constants.ContactShadowLength = LightSceneInfo->Proxy->GetContactShadowLength();
		// Sign indicates if contact shadow length is in world space or screen space.
		// Multiply by 2 for screen space in order to preserve old values after introducing multiply by View.ClipToView[1][1] in shader.
		DeferredLightUniformsValue.Constants.ContactShadowLength *= LightSceneInfo->Proxy->IsContactShadowLengthInWS() ? -1.0f : 2.0f;
	}

	// When rendering reflection captures, the direct lighting of the light is actually the indirect specular from the main view
// 	if (View.bIsReflectionCapture)
// 	{
// 		DeferredLightUniformsValue.LightColor *= LightSceneInfo->Proxy->GetIndirectLightingScale();
// 	}

	const ELightComponentType LightType = (ELightComponentType)LightSceneInfo->Proxy->GetLightType();
	if ((LightType == LightType_Point || LightType == LightType_Spot || LightType == LightType_Rect) && View.IsPerspectiveProjection())
	{
		DeferredLightUniformsValue.Constants.LightColor *= GetLightFadeFactor(View, LightSceneInfo->Proxy);
	}

	DeferredLightUniformsValue.Constants.LightingChannelMask = LightSceneInfo->Proxy->GetLightingChannelMask();

	SetUniformBufferParameterImmediate(ShaderRHI, DeferredLightUniformBufferParameter, DeferredLightUniformsValue);
}

/** Utility functions for drawing a sphere */
namespace StencilingGeometry
{
	/**
	* Draws a sphere using RHIDrawIndexedPrimitive, useful as approximate bounding geometry for deferred passes.
	* Note: The sphere will be of unit size unless transformed by the shader.
	*/
	extern void DrawSphere();
	/**
	* Draws exactly the same as above, but uses FVector rather than FVector4 vertex data.
	*/
	extern void DrawVectorSphere();
	/** Renders a cone with a spherical cap, used for rendering spot lights in deferred passes. */
	extern void DrawCone();

	template<int32 NumSphereSides, int32 NumSphereRings, typename VectorType>
	class TStencilSphereVertexBuffer 
	{
	public:

		int32 GetNumRings() const
		{
			return NumSphereRings;
		}

		/**
		* Initialize the RHI for this rendering resource
		*/
		void InitRHI() 
		{
			const int32 NumSides = NumSphereSides;
			const int32 NumRings = NumSphereRings;
			const int32 NumVerts = (NumSides + 1) * (NumRings + 1);

			const float RadiansPerRingSegment = PI / (float)NumRings;
			float Radius = 1;

			std::vector<VectorType> ArcVerts;
			ArcVerts.reserve(NumRings + 1);
			// Calculate verts for one arc
			for (int32 i = 0; i < NumRings + 1; i++)
			{
				const float Angle = i * RadiansPerRingSegment;
				ArcVerts.push_back(FVector(0.0f, FMath::Sin(Angle), FMath::Cos(Angle)));
			}

			std::vector<VectorType> Verts;
			Verts.reserve(NumVerts);
			// Then rotate this arc NumSides + 1 times.
			const FVector Center = FVector(0, 0, 0);
			for (int32 s = 0; s < NumSides + 1; s++)
			{
				FRotator ArcRotator(0, 360.f * ((float)s / NumSides), 0);
				FRotationMatrix ArcRot(ArcRotator);

				for (int32 v = 0; v < NumRings + 1; v++)
				{
					const int32 VIx = (NumRings + 1) * s + v;
					Verts.push_back(Center + Radius * ArcRot.TransformPosition(ArcVerts[v]));
				}
			}

			NumSphereVerts = (int32)Verts.size();
			uint32 Size = Verts.size() * sizeof(VectorType);

			// Create vertex buffer. Fill buffer with initial data upon creation
			//FRHIResourceCreateInfo CreateInfo(&Verts);
			VertexBufferRHI = RHICreateVertexBuffer(Size, D3D11_USAGE_DEFAULT,0,0, Verts.data());
		}

		int32 GetVertexCount() const { return NumSphereVerts; }

		/**
		* Calculates the world transform for a sphere.
		* @param OutTransform - The output world transform.
		* @param Sphere - The sphere to generate the transform for.
		* @param PreViewTranslation - The pre-view translation to apply to the transform.
		* @param bConservativelyBoundSphere - when true, the sphere that is drawn will contain all positions in the analytical sphere,
		*		 Otherwise the sphere vertices will lie on the analytical sphere and the positions on the faces will lie inside the sphere.
		*/
		void CalcTransform(Vector4& OutPosAndScale, const FSphere& Sphere, const FVector& PreViewTranslation, bool bConservativelyBoundSphere = true)
		{
			float Radius = Sphere.W;
			if (bConservativelyBoundSphere)
			{
				const int32 NumRings = NumSphereRings;
				const float RadiansPerRingSegment = PI / (float)NumRings;

				// Boost the effective radius so that the edges of the sphere approximation lie on the sphere, instead of the vertices
				Radius /= FMath::Cos(RadiansPerRingSegment);
			}

			const FVector Translate(Sphere.Center + PreViewTranslation);
			OutPosAndScale = Vector4(Translate, Radius);
		}

	private:
		int32 NumSphereVerts;
		ID3D11Buffer* VertexBufferRHI;
	};

	/**
	* Stenciling sphere index buffer
	*/
	template<int32 NumSphereSides, int32 NumSphereRings>
	class TStencilSphereIndexBuffer 
	{
	public:
		/**
		* Initialize the RHI for this rendering resource
		*/
		void InitRHI() 
		{
			const int32 NumSides = NumSphereSides;
			const int32 NumRings = NumSphereRings;
			std::vector<uint32> Indices;

			// Add triangles for all the vertices generated
			for (int32 s = 0; s < NumSides; s++)
			{
				const int32 a0start = (s + 0) * (NumRings + 1);
				const int32 a1start = (s + 1) * (NumRings + 1);

				for (int32 r = 0; r < NumRings; r++)
				{
					Indices.push_back(a0start + r + 0);
					Indices.push_back(a1start + r + 0);
					Indices.push_back(a0start + r + 1);
					Indices.push_back(a1start + r + 0);
					Indices.push_back(a1start + r + 1);
					Indices.push_back(a0start + r + 1);
				}
			}

			NumIndices = Indices.size();
			const uint32 Size = Indices.size() * sizeof(uint32);
			const uint32 Stride = sizeof(uint32);

			// Create index buffer. Fill buffer with initial data upon creation
			//FRHIResourceCreateInfo CreateInfo(&Indices);
			IndexBufferRHI = CreateIndexBuffer(Indices.data(), Size); //RHICreateIndexBuffer(Stride, Size, BUF_Static, CreateInfo);
			
		}

		int32 GetIndexCount() const { return NumIndices; };

	private:
		int32 NumIndices;
		ID3D11Buffer* IndexBufferRHI;
	};

	class FStencilConeIndexBuffer 
	{
	public:
		// A side is a line of vertices going from the cone's origin to the edge of its SphereRadius
		static const int32 NumSides = 18;
		// A slice is a circle of vertices in the cone's XY plane
		static const int32 NumSlices = 12;

		static const uint32 NumVerts = NumSides * NumSlices * 2;

		void InitRHI() 
		{
			std::vector<uint32> Indices;

			Indices.reserve((NumSlices - 1) * NumSides * 12);
			// Generate triangles for the vertices of the cone shape
			for (int32 SliceIndex = 0; SliceIndex < NumSlices - 1; SliceIndex++)
			{
				for (int32 SideIndex = 0; SideIndex < NumSides; SideIndex++)
				{
					const int32 CurrentIndex = SliceIndex * NumSides + SideIndex % NumSides;
					const int32 NextSideIndex = SliceIndex * NumSides + (SideIndex + 1) % NumSides;
					const int32 NextSliceIndex = (SliceIndex + 1) * NumSides + SideIndex % NumSides;
					const int32 NextSliceAndSideIndex = (SliceIndex + 1) * NumSides + (SideIndex + 1) % NumSides;

					Indices.push_back(CurrentIndex);
					Indices.push_back(NextSideIndex);
					Indices.push_back(NextSliceIndex);
					Indices.push_back(NextSliceIndex);
					Indices.push_back(NextSideIndex);
					Indices.push_back(NextSliceAndSideIndex);
				}
			}

			// Generate triangles for the vertices of the spherical cap
			const int32 CapIndexStart = NumSides * NumSlices;

			for (int32 SliceIndex = 0; SliceIndex < NumSlices - 1; SliceIndex++)
			{
				for (int32 SideIndex = 0; SideIndex < NumSides; SideIndex++)
				{
					const int32 CurrentIndex = SliceIndex * NumSides + SideIndex % NumSides + CapIndexStart;
					const int32 NextSideIndex = SliceIndex * NumSides + (SideIndex + 1) % NumSides + CapIndexStart;
					const int32 NextSliceIndex = (SliceIndex + 1) * NumSides + SideIndex % NumSides + CapIndexStart;
					const int32 NextSliceAndSideIndex = (SliceIndex + 1) * NumSides + (SideIndex + 1) % NumSides + CapIndexStart;

					Indices.push_back(CurrentIndex);
					Indices.push_back(NextSliceIndex);
					Indices.push_back(NextSideIndex);
					Indices.push_back(NextSideIndex);
					Indices.push_back(NextSliceIndex);
					Indices.push_back(NextSliceAndSideIndex);
				}
			}

			const uint32 Size = Indices.size() * sizeof(uint32);
			const uint32 Stride = sizeof(uint32);

			NumIndices = Indices.size();

			// Create index buffer. Fill buffer with initial data upon creation
			//FRHIResourceCreateInfo CreateInfo(&Indices);
			IndexBufferRHI = CreateIndexBuffer(Indices.data(), Size);// RHICreateIndexBuffer(Stride, Size, BUF_Static, CreateInfo);
		}

		int32 GetIndexCount() const { return NumIndices; }

	protected:
		int32 NumIndices;
		ID3D11Buffer* IndexBufferRHI;
	};

	/**
	* Vertex buffer for a cone. It holds zero'd out data since the actual math is done on the shader
	*/
	class FStencilConeVertexBuffer 
	{
	public:
		static const int32 NumVerts = FStencilConeIndexBuffer::NumSides * FStencilConeIndexBuffer::NumSlices * 2;

		/**
		* Initialize the RHI for this rendering resource
		*/
		void InitRHI() 
		{
			std::vector<Vector4> Verts;
			Verts.reserve(NumVerts);
			for (int32 s = 0; s < NumVerts; s++)
			{
				Verts.push_back(Vector4(0, 0, 0, 0));
			}

			uint32 Size = Verts.size() * sizeof(Vector4);

			// Create vertex buffer. Fill buffer with initial data upon creation
			//FRHIResourceCreateInfo CreateInfo(&Verts);
			VertexBufferRHI = RHICreateVertexBuffer(Size, D3D11_USAGE_DEFAULT, 0, 0, Verts.data());
		}

		int32 GetVertexCount() const { return NumVerts; }
	private:
		ID3D11Buffer * VertexBufferRHI;
	};

	extern TStencilSphereVertexBuffer<18, 12, Vector4> 		GStencilSphereVertexBuffer;
	extern TStencilSphereVertexBuffer<18, 12, FVector> 		GStencilSphereVectorBuffer;
	extern TStencilSphereIndexBuffer<18, 12>				GStencilSphereIndexBuffer;
	extern TStencilSphereVertexBuffer<4, 4, Vector4> 		GLowPolyStencilSphereVertexBuffer;
	extern TStencilSphereIndexBuffer<4, 4> 					GLowPolyStencilSphereIndexBuffer;
	extern FStencilConeVertexBuffer							GStencilConeVertexBuffer;
	extern FStencilConeIndexBuffer							GStencilConeIndexBuffer;
}
/**
* Stencil geometry parameters used by multiple shaders.
*/
class FStencilingGeometryShaderParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		StencilGeometryPosAndScale.Bind(ParameterMap, ("StencilingGeometryPosAndScale"));
		StencilConeParameters.Bind(ParameterMap, ("StencilingConeParameters"));
		StencilConeTransform.Bind(ParameterMap, ("StencilingConeTransform"));
		StencilPreViewTranslation.Bind(ParameterMap, ("StencilingPreViewTranslation"));
	}

	void Set(FShader* Shader, const Vector4& InStencilingGeometryPosAndScale) const
	{
		SetShaderValue(Shader->GetVertexShader(), StencilGeometryPosAndScale, InStencilingGeometryPosAndScale);
		SetShaderValue(Shader->GetVertexShader(), StencilConeParameters, Vector4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	void Set(FShader* Shader, const FSceneView& View, const FLightSceneInfo* LightSceneInfo) const
	{
		Vector4 GeometryPosAndScale;
		if (LightSceneInfo->Proxy->GetLightType() == LightType_Point ||
			LightSceneInfo->Proxy->GetLightType() == LightType_Rect)
		{
			StencilingGeometry::GStencilSphereVertexBuffer.CalcTransform(GeometryPosAndScale, LightSceneInfo->Proxy->GetBoundingSphere(), View.ViewMatrices.GetPreViewTranslation());
			SetShaderValue(Shader->GetVertexShader(), StencilGeometryPosAndScale, GeometryPosAndScale);
			SetShaderValue(Shader->GetVertexShader(), StencilConeParameters, Vector4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		else if (LightSceneInfo->Proxy->GetLightType() == LightType_Spot)
		{
			SetShaderValue(Shader->GetVertexShader(), StencilConeTransform, LightSceneInfo->Proxy->GetLightToWorld());
			SetShaderValue(
				Shader->GetVertexShader(),
				StencilConeParameters,
				Vector4(
					StencilingGeometry::FStencilConeIndexBuffer::NumSides,
					StencilingGeometry::FStencilConeIndexBuffer::NumSlices,
					LightSceneInfo->Proxy->GetOuterConeAngle(),
					LightSceneInfo->Proxy->GetRadius()));
			SetShaderValue(Shader->GetVertexShader(), StencilPreViewTranslation, View.ViewMatrices.GetPreViewTranslation());
		}
	}

private:
	FShaderParameter StencilGeometryPosAndScale;
	FShaderParameter StencilConeParameters;
	FShaderParameter StencilConeTransform;
	FShaderParameter StencilPreViewTranslation;
};