#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "SceneManagement.h"
#include "RenderTargetPool.h"
#include "LightSceneInfo.h"
#include "SceneCore.h"
#include "Material.h"
#include "DrawingPolicy.h"
#include "GlobalShader.h"
#include "LightRendering.h"
#include "SystemTextures.h"

#include <functional>

class FPrimitiveSceneInfo;
class FProjectedShadowInfo;
class FViewInfo;
class FMaterialRenderProxy;
template <bool bRenderingReflectiveShadowMaps> class TShadowDepthBasePS;
class FShadowStaticMeshElement;

/**
* The shadow depth drawing policy's context data.
*/
struct FShadowDepthDrawingPolicyContext : FMeshDrawingPolicy::ContextDataType
{
	/** CAUTION, this is assumed to be a POD type. We allocate the on the scene allocator and NEVER CALL A DESTRUCTOR.
	If you want to add non-pod data, not a huge problem, we just need to track and destruct them at the end of the scene.
	**/
	/** The projected shadow info for which we are rendering shadow depths. */
	const FProjectedShadowInfo* ShadowInfo;

	/** Initialization constructor. */
	explicit FShadowDepthDrawingPolicyContext(const FProjectedShadowInfo* InShadowInfo)
		: ShadowInfo(InShadowInfo)
	{}
};
/**
* Outputs no color, but can be used to write the mesh's depth values to the depth buffer.
*/
template <bool bRenderingReflectiveShadowMaps>
class FShadowDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:
	typedef FShadowDepthDrawingPolicyContext ContextDataType;

	FShadowDepthDrawingPolicy(
		const FMaterial* InMaterialResource,
		bool bInDirectionalLight,
		bool bInOnePassPointLightShadow,
		bool bInPreShadow,
		const FMeshDrawingPolicyOverrideSettings& InOverrideSettings,
		const FVertexFactory* InVertexFactory = 0,
		const FMaterialRenderProxy* InMaterialRenderProxy = 0,
		bool bReverseCulling = false
	);

	void UpdateElementState(FShadowStaticMeshElement& State);

	FShadowDepthDrawingPolicy& operator = (const FShadowDepthDrawingPolicy& Other)
	{
		VertexShader = Other.VertexShader;
		GeometryShader = Other.GeometryShader;
		HullShader = Other.HullShader;
		DomainShader = Other.DomainShader;
		PixelShader = Other.PixelShader;
		bDirectionalLight = Other.bDirectionalLight;
		bReverseCulling = Other.bReverseCulling;
		bOnePassPointLightShadow = Other.bOnePassPointLightShadow;
		bUsePositionOnlyVS = Other.bUsePositionOnlyVS;
		bPreShadow = Other.bPreShadow;
		(FMeshDrawingPolicy&)*this = (const FMeshDrawingPolicy&)Other;
		return *this;
	}

	void SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View, const ContextDataType PolicyContext) const;

	/**
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @return new bound shader state object
	*/
	FBoundShaderStateInput GetBoundShaderStateInput() const;

	void SetMeshRenderState(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		const FDrawingPolicyRenderState& DrawRenderState,
// 		const ElementDataType& ElementData,
		const ContextDataType PolicyContext
	) const;

	template<bool T2>
	friend int32 CompareDrawingPolicy(const FShadowDepthDrawingPolicy<T2>& A, const FShadowDepthDrawingPolicy<T2>& B);

	bool IsReversingCulling() const
	{
		return bReverseCulling;
	}

private:

	class FShadowDepthVS* VertexShader;
	class FOnePassPointShadowDepthGS* GeometryShader;
	TShadowDepthBasePS<bRenderingReflectiveShadowMaps>* PixelShader;
	class FBaseHS* HullShader;
	class FShadowDepthDS* DomainShader;

public:

	uint32 bDirectionalLight : 1;
	uint32 bReverseCulling : 1;
	uint32 bOnePassPointLightShadow : 1;
	uint32 bUsePositionOnlyVS : 1;
	uint32 bPreShadow : 1;
};



/**
* A drawing policy factory for the shadow depth drawing policy.
*/
class FShadowDepthDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = false };

	struct ContextType
	{
		const FProjectedShadowInfo* ShadowInfo;

		ContextType(const FProjectedShadowInfo* InShadowInfo) :
			ShadowInfo(InShadowInfo)
		{}
	};

	static void AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh);

	static bool DrawDynamicMesh(
		const FSceneView& View,
		ContextType Context,
		const FMeshBatch& Mesh,
		bool bPreFog,
		const FDrawingPolicyRenderState& DrawRenderState,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy
	);
};

/** A single static mesh element for shadow depth rendering. */
class FShadowStaticMeshElement
{
public:

	FShadowStaticMeshElement()
		: RenderProxy(0)
		, MaterialResource(0)
		, Mesh(0)
		, bIsTwoSided(false)
	{
	}

	FShadowStaticMeshElement(const FMaterialRenderProxy* InRenderProxy, const FMaterial* InMaterialResource, const FStaticMesh* InMesh, bool bInIsTwoSided) :
		RenderProxy(InRenderProxy),
		MaterialResource(InMaterialResource),
		Mesh(InMesh),
		bIsTwoSided(bInIsTwoSided)
	{}

	bool DoesDeltaRequireADrawSharedCall(const FShadowStaticMeshElement& rhs) const
	{
		assert(rhs.RenderProxy);
		assert(rhs.Mesh);

		// Note: this->RenderProxy or this->Mesh can be 0
		// but in this case rhs.RenderProxy should not be 0
		// so it will early out and there will be no crash on Mesh->VertexFactory
		assert(!RenderProxy || rhs.RenderProxy);

		return RenderProxy != rhs.RenderProxy
			|| bIsTwoSided != rhs.bIsTwoSided
			|| Mesh->VertexFactory != rhs.Mesh->VertexFactory
			|| Mesh->ReverseCulling != rhs.Mesh->ReverseCulling;
	}

	/** Store the FMaterialRenderProxy pointer since it may be different from the one that FStaticMesh stores. */
	const FMaterialRenderProxy* RenderProxy;
	const FMaterial* MaterialResource;
	const FStaticMesh* Mesh;
	bool bIsTwoSided;
};


enum EShadowDepthRenderMode
{
	/** The render mode used by regular shadows */
	ShadowDepthRenderMode_Normal,

	/** The render mode used when injecting emissive-only objects into the RSM. */
	ShadowDepthRenderMode_EmissiveOnly,

	/** The render mode used when rendering volumes which block global illumination. */
	ShadowDepthRenderMode_GIBlockingVolumes,
};

enum EShadowDepthCacheMode
{
	SDCM_MovablePrimitivesOnly,
	SDCM_StaticPrimitivesOnly,
	SDCM_Uncached
};

class FShadowMapRenderTargets
{
public:
	std::vector<PooledRenderTarget*> ColorTargets;
	PooledRenderTarget* DepthTarget;

	FShadowMapRenderTargets() :
		DepthTarget(NULL)
	{}

	FIntPoint GetSize() const
	{
		if (DepthTarget)
		{
			return DepthTarget->GetDesc().Extent;
		}
		else
		{
			assert(ColorTargets.size() > 0);
			return ColorTargets[0]->GetDesc().Extent;
		}
	}
};

struct FShadowProjectionMatrix : FMatrix
{
	FShadowProjectionMatrix(float MinZ, float MaxZ, const Vector4& WAxis) :
		FMatrix(
			FPlane(1, 0, 0, WAxis.X),
			FPlane(0, 1, 0, WAxis.Y),
			FPlane(0, 0, (WAxis.Z * MaxZ + WAxis.W) / (MaxZ - MinZ), WAxis.Z),
			FPlane(0, 0, -MinZ * (WAxis.Z * MaxZ + WAxis.W) / (MaxZ - MinZ), WAxis.W)
		)
	{}
};

typedef std::function<void(bool bFirst)> FSetShadowRenderTargetFunction;
/**
* Information about a projected shadow.
*/
class FProjectedShadowInfo 
{
public:
	typedef std::vector<const FPrimitiveSceneInfo*> PrimitiveArrayType;

	/** The view to be used when rendering this shadow's depths. */
	FViewInfo * ShadowDepthView;

	/** The depth or color targets this shadow was rendered to. */
	FShadowMapRenderTargets RenderTargets;

	EShadowDepthCacheMode CacheMode;

	/** The main view this shadow must be rendered in, or NULL for a view independent shadow. */
	FViewInfo* DependentView;

	/** Index of the shadow into FVisibleLightInfo::AllProjectedShadows. */
	int32 ShadowId;

	/** A translation that is applied to world-space before transforming by one of the shadow matrices. */
	FVector PreShadowTranslation;

	/** The effective view matrix of the shadow, used as an override to the main view's view matrix when rendering the shadow depth pass. */
	FMatrix ShadowViewMatrix;

	/**
	* Matrix used for rendering the shadow depth buffer.
	* Note that this does not necessarily contain all of the shadow casters with CSM, since the vertex shader flattens them onto the near plane of the projection.
	*/
	FMatrix SubjectAndReceiverMatrix;
	FMatrix ReceiverMatrix;

	FMatrix InvReceiverMatrix;

	float InvMaxSubjectDepth;

	/**
	* Subject depth extents, in world space units.
	* These can be used to convert shadow depth buffer values back into world space units.
	*/
	float MaxSubjectZ;
	float MinSubjectZ;

	/** Frustum containing all potential shadow casters. */
	FConvexVolume CasterFrustum;
	FConvexVolume ReceiverFrustum;

	float MinPreSubjectZ;

	FSphere ShadowBounds;

	FShadowCascadeSettings CascadeSettings;

	/**
	* X and Y position of the shadow in the appropriate depth buffer.  These are only initialized after the shadow has been allocated.
	* The actual contents of the shadowmap are at X + BorderSize, Y + BorderSize.
	*/
	uint32 X;
	uint32 Y;

	/**
	* Resolution of the shadow, excluding the border.
	* The full size of the region allocated to this shadow is therefore ResolutionX + 2 * BorderSize, ResolutionY + 2 * BorderSize.
	*/
	uint32 ResolutionX;
	uint32 ResolutionY;

	/** Size of the border, if any, used to allow filtering without clamping for shadows stored in an atlas. */
	uint32 BorderSize;

	/** The largest percent of either the width or height of any view. */
	float MaxScreenPercent;

	/** Fade Alpha per view. */
	std::vector<float> FadeAlphas;

	/** Whether the shadow has been allocated in the shadow depth buffer, and its X and Y properties have been initialized. */
	uint32 bAllocated : 1;

	/** Whether the shadow's projection has been rendered. */
	uint32 bRendered : 1;

	/** Whether the shadow has been allocated in the preshadow cache, so its X and Y properties offset into the preshadow cache depth buffer. */
	uint32 bAllocatedInPreshadowCache : 1;

	/** Whether the shadow is in the preshadow cache and its depths are up to date. */
	uint32 bDepthsCached : 1;

	// redundant to LightSceneInfo->Proxy->GetLightType() == LightType_Directional, could be made ELightComponentType LightType
	uint32 bDirectionalLight : 1;

	/** Whether the shadow is a point light shadow that renders all faces of a cubemap in one pass. */
	uint32 bOnePassPointLightShadow : 1;

	/** Whether this shadow affects the whole scene or only a group of objects. */
	uint32 bWholeSceneShadow : 1;

	/** Whether the shadow needs to render reflective shadow maps. */
	uint32 bReflectiveShadowmap : 1;

	/** Whether this shadow should support casting shadows from translucent surfaces. */
	uint32 bTranslucentShadow : 1;

	/** Whether the shadow will be computed by ray tracing the distance field. */
	uint32 bRayTracedDistanceField : 1;

	/** Whether this is a per-object shadow that should use capsule shapes to shadow instead of the mesh's triangles. */
	uint32 bCapsuleShadow : 1;

	/** Whether the shadow is a preshadow or not.  A preshadow is a per object shadow that handles the static environment casting on a dynamic receiver. */
	uint32 bPreShadow : 1;

	/** To not cast a shadow on the ground outside the object and having higher quality (useful for first person weapon). */
	uint32 bSelfShadowOnly : 1;

	/** Whether the shadow is a per object shadow or not. */
	uint32 bPerObjectOpaqueShadow : 1;

	/** Whether turn on back-lighting transmission. */
	uint32 bTransmission : 1;

	//TBitArray<SceneRenderingBitArrayAllocator> StaticMeshWholeSceneShadowDepthMap;
	std::vector<uint64> StaticMeshWholeSceneShadowBatchVisibility;

	/** View projection matrices for each cubemap face, used by one pass point light shadows. */
	std::vector<FMatrix> OnePassShadowViewProjectionMatrices;

	/** Frustums for each cubemap face, used for object culling one pass point light shadows. */
	std::vector<FConvexVolume> OnePassShadowFrustums;

	/** Data passed from async compute begin to end. */
	//FComputeFenceRHIRef RayTracedShadowsEndFence;
	ComPtr<PooledRenderTarget> RayTracedShadowsRT;


	
public:
	// default constructor
	FProjectedShadowInfo();

	bool SetupPerObjectProjection(
		FLightSceneInfo* InLightSceneInfo,
		const FPrimitiveSceneInfo* InParentSceneInfo,
		const FPerObjectProjectedShadowInitializer& Initializer,
		bool bInPreShadow,
		uint32 InResolutionX,
		uint32 MaxShadowResolutionY,
		uint32 InBorderSize,
		float InMaxScreenPercent,
		bool bInTranslucentShadow
	);

	/** for a whole-scene shadow. */
	void SetupWholeSceneProjection(
		FLightSceneInfo* InLightSceneInfo,
		FViewInfo* InDependentView,
		const FWholeSceneProjectedShadowInitializer& Initializer,
		uint32 InResolutionX,
		uint32 InResolutionY,
		uint32 InBorderSize,
		bool bInReflectiveShadowMap
	);

	float GetShaderDepthBias() const { return ShaderDepthBias; }

	void RenderDepth(class FSceneRenderer* SceneRenderer, FSetShadowRenderTargetFunction SetShadowRenderTargets, EShadowDepthRenderMode RenderMode);

	void SetStateForDepth(EShadowDepthRenderMode RenderMode, FDrawingPolicyRenderState& DrawRenderState);

	static void SetBlendStateForProjection(
		int32 ShadowMapChannel,
		bool bIsWholeSceneDirectionalShadow,
		bool bUseFadePlane,
		bool bProjectingForForwardShading,
		bool bMobileModulatedProjections);

	void SetBlendStateForProjection(bool bProjectingForForwardShading, bool bMobileModulatedProjections) const;

	void RenderProjection(int32 ViewIndex, const class FViewInfo* View, const class FSceneRenderer* SceneRender, bool bProjectingForForwardShading, bool bMobile) const;
	void RenderOnePassPointLightProjection(int32 ViewIndex, const FViewInfo& View, bool bProjectingForForwardShading) const;

	void AddSubjectPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo, std::vector<FViewInfo>* ViewArray, bool bRecordShadowSubjectForMobileShading);

	bool HasSubjectPrims() const;

	void SetupShadowDepthView(FSceneRenderer* SceneRenderer);

	const FLightSceneInfo& GetLightSceneInfo() const { return *LightSceneInfo; }
	const FLightSceneInfoCompact& GetLightSceneInfoCompact() const { return LightSceneInfoCompact; }

	void UpdateShaderDepthBias();

	inline bool IsWholeSceneDirectionalShadow() const
	{
		return bWholeSceneShadow && CascadeSettings.ShadowSplitIndex >= 0 && bDirectionalLight;
	}

	inline bool IsWholeScenePointLightShadow() const
	{
		return bWholeSceneShadow && (LightSceneInfo->Proxy->GetLightType() == LightType_Point || LightSceneInfo->Proxy->GetLightType() == LightType_Rect);
	}
private:
	const FLightSceneInfo* LightSceneInfo;
	FLightSceneInfoCompact LightSceneInfoCompact;

	/** Static shadow casting elements. */
	std::vector<FShadowStaticMeshElement> StaticSubjectMeshElements;

	/** Dynamic mesh elements for subject primitives. */
	std::vector<FMeshBatchAndRelevance> DynamicSubjectMeshElements;

	PrimitiveArrayType DynamicSubjectPrimitives;
	PrimitiveArrayType ReceiverPrimitives;

	float ShaderDepthBias;

	void CopyCachedShadowMap(const FDrawingPolicyRenderState& DrawRenderState, FSceneRenderer* SceneRenderer, const FViewInfo& View, FSetShadowRenderTargetFunction SetShadowRenderTargets);

	/**
	* Renders the shadow subject depth, to a particular hacked view
	*/
	void RenderDepthInner(class FSceneRenderer* SceneRenderer, const FViewInfo* FoundView, FSetShadowRenderTargetFunction SetShadowRenderTargets, EShadowDepthRenderMode RenderMode);
	/**
	* Modifies the passed in view for this shadow
	*/
	void ModifyViewForShadow(FViewInfo* FoundView) const;
	/**
	* Finds a relevant view for a shadow
	*/
	FViewInfo* FindViewForShadow(FSceneRenderer* SceneRenderer) const;

	void RenderDepthDynamic(class FSceneRenderer* SceneRenderer, const FViewInfo* FoundView, const FDrawingPolicyRenderState& DrawRenderState);

	template <bool bReflectiveShadowmap> friend void DrawShadowMeshElements(const FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState, const FProjectedShadowInfo& ShadowInfo);

	friend class FShadowDepthVS;
	template <bool bRenderingReflectiveShadowMaps> friend class TShadowDepthBasePS;
	friend class FShadowVolumeBoundProjectionVS;
	friend class FShadowProjectionPS;
	friend class FShadowDepthDrawingPolicyFactory;
};

/** Shader parameters for rendering the depth of a mesh for shadowing. */
class FShadowDepthShaderParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		ProjectionMatrix.Bind(ParameterMap, ("ProjectionMatrix"));
		ShadowParams.Bind(ParameterMap, ("ShadowParams"));
		ClampToNearPlane.Bind(ParameterMap, ("bClampToNearPlane"));
	}

	template<typename ShaderRHIParamRef>
	void Set(ShaderRHIParamRef ShaderRHI, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, const FMaterialRenderProxy* MaterialRenderProxy)
	{
		SetShaderValue(
			ShaderRHI,
			ProjectionMatrix,
			FTranslationMatrix(ShadowInfo->PreShadowTranslation - View.ViewMatrices.GetPreViewTranslation()) * ShadowInfo->SubjectAndReceiverMatrix
		);

		SetShaderValue(ShaderRHI, ShadowParams, Vector2(ShadowInfo->GetShaderDepthBias(), ShadowInfo->InvMaxSubjectDepth));
		// Only clamp vertices to the near plane when rendering whole scene directional light shadow depths or preshadows from directional lights
		const bool bClampToNearPlaneValue = ShadowInfo->IsWholeSceneDirectionalShadow() || (ShadowInfo->bPreShadow && ShadowInfo->bDirectionalLight);
		SetShaderValue(ShaderRHI, ClampToNearPlane, bClampToNearPlaneValue ? 1.0f : 0.0f);
	}

	/** Set the vertex shader parameter values. */
	void SetVertexShader(FShader* VertexShader, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, const FMaterialRenderProxy* MaterialRenderProxy)
	{
		Set(VertexShader->GetVertexShader(), View, ShadowInfo, MaterialRenderProxy);
	}

	/** Set the domain shader parameter values. */
	void SetDomainShader(FShader* DomainShader, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, const FMaterialRenderProxy* MaterialRenderProxy)
	{
		Set(DomainShader->GetDomainShader(), View, ShadowInfo, MaterialRenderProxy);
	}


private:
	FShaderParameter ProjectionMatrix;
	FShaderParameter ShadowParams;
	FShaderParameter ClampToNearPlane;
};

/**
* A generic vertex shader for projecting a shadow depth buffer onto the scene.
*/
class FShadowProjectionVertexShaderInterface : public FGlobalShader
{
public:
	FShadowProjectionVertexShaderInterface() {}
	FShadowProjectionVertexShaderInterface(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{ }

	virtual void SetParameters(const FSceneView& View, const FProjectedShadowInfo* ShadowInfo) = 0;
};

/**
* A vertex shader for projecting a shadow depth buffer onto the scene.
*/
class FShadowVolumeBoundProjectionVS : public FShadowProjectionVertexShaderInterface
{
	DECLARE_SHADER_TYPE(FShadowVolumeBoundProjectionVS, Global);
public:

	FShadowVolumeBoundProjectionVS() {}
	FShadowVolumeBoundProjectionVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FShadowProjectionVertexShaderInterface(Initializer)
	{
		StencilingGeometryParameters.Bind(Initializer.ParameterMap);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FShadowProjectionVertexShaderInterface::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("USE_TRANSFORM"), (uint32)1);
	}

	void SetParameters(const FSceneView& View, const FProjectedShadowInfo* ShadowInfo) override;


private:
	FStencilingGeometryShaderParameters StencilingGeometryParameters;
};

/** One pass point light shadow projection parameters used by multiple shaders. */
class FOnePassPointShadowProjectionShaderParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		ShadowDepthTexture.Bind(ParameterMap, ("ShadowDepthCubeTexture"));
		ShadowDepthTexture2.Bind(ParameterMap, ("ShadowDepthCubeTexture2"));
		ShadowDepthCubeComparisonSampler.Bind(ParameterMap, ("ShadowDepthCubeTextureSampler"));
		ShadowViewProjectionMatrices.Bind(ParameterMap, ("ShadowViewProjectionMatrices"));
		InvShadowmapResolution.Bind(ParameterMap, ("InvShadowmapResolution"));
	}

	template<typename ShaderRHIParamRef>
	void Set(const ShaderRHIParamRef ShaderRHI, const FProjectedShadowInfo* ShadowInfo) const
	{
		FD3D11Texture2D* ShadowDepthTextureValue = ShadowInfo
			? ShadowInfo->RenderTargets.DepthTarget->ShaderResourceTexture.get()
			: GBlackTextureDepthCube.get();
		if (!ShadowDepthTextureValue)
		{
			ShadowDepthTextureValue = GBlackTextureDepthCube.get();
		}

		SetTextureParameter(
			ShaderRHI,
			ShadowDepthTexture,
			ShadowDepthTextureValue->GetShaderResourceView()
		);

		SetTextureParameter(
			ShaderRHI,
			ShadowDepthTexture2,
			ShadowDepthTextureValue->GetShaderResourceView()
		);

		if (ShadowDepthCubeComparisonSampler.IsBound())
		{
			SetShaderSampler(
				ShaderRHI,
				ShadowDepthCubeComparisonSampler.GetBaseIndex(),
				// Use a comparison sampler to do hardware PCF
				TStaticSamplerState<D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 0, 0, D3D11_COMPARISON_LESS>::GetRHI()
			);
		}

		if (ShadowInfo)
		{
			SetShaderValueArray<ShaderRHIParamRef, FMatrix>(
				ShaderRHI,
				ShadowViewProjectionMatrices,
				ShadowInfo->OnePassShadowViewProjectionMatrices.data(),
				ShadowInfo->OnePassShadowViewProjectionMatrices.size()
				);

			SetShaderValue(ShaderRHI, InvShadowmapResolution, 1.0f / ShadowInfo->ResolutionX);
		}
		else
		{
			std::vector<FMatrix> ZeroMatrices;
			ZeroMatrices.resize(FMath::DivideAndRoundUp<int32>(ShadowViewProjectionMatrices.GetNumBytes(), sizeof(FMatrix)));

			SetShaderValueArray<ShaderRHIParamRef, FMatrix>(
				ShaderRHI,
				ShadowViewProjectionMatrices,
				ZeroMatrices.data(),
				ZeroMatrices.size()
				);

			SetShaderValue(ShaderRHI, InvShadowmapResolution, 0);
		}
	}

private:
	FShaderResourceParameter ShadowDepthTexture;
	FShaderResourceParameter ShadowDepthTexture2;
	FShaderResourceParameter ShadowDepthCubeComparisonSampler;
	FShaderParameter ShadowViewProjectionMatrices;
	FShaderParameter InvShadowmapResolution;
};

/**
* Pixel shader used to project one pass point light shadows.
*/
// Quality = 0 / 1
template <uint32 Quality, bool bUseTransmission = false>
class TOnePassPointShadowProjectionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TOnePassPointShadowProjectionPS, Global);
public:

	TOnePassPointShadowProjectionPS() {}

	TOnePassPointShadowProjectionPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer);
		OnePassShadowParameters.Bind(Initializer.ParameterMap);
		ShadowDepthTextureSampler.Bind(Initializer.ParameterMap, ("ShadowDepthTextureSampler"));
		LightPosition.Bind(Initializer.ParameterMap, ("LightPositionAndInvRadius"));
		ShadowFadeFraction.Bind(Initializer.ParameterMap, ("ShadowFadeFraction"));
		ShadowSharpen.Bind(Initializer.ParameterMap, ("ShadowSharpen"));
		PointLightDepthBiasAndProjParameters.Bind(Initializer.ParameterMap, ("PointLightDepthBiasAndProjParameters"));
		TransmissionProfilesTexture.Bind(Initializer.ParameterMap, ("SSProfilesTexture"));
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("SHADOW_QUALITY"), Quality);
		OutEnvironment.SetDefine(("USE_TRANSMISSION"), (uint32)(bUseTransmission ? 1 : 0));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;// IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	void SetParameters(
		int32 ViewIndex,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo
	)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer.get());

		SceneTextureParameters.Set(ShaderRHI, ESceneTextureSetupMode::All);
		OnePassShadowParameters.Set(ShaderRHI, ShadowInfo);

		const FLightSceneProxy& LightProxy = *(ShadowInfo->GetLightSceneInfo().Proxy);

		SetShaderValue(ShaderRHI, LightPosition, Vector4(LightProxy.GetPosition(), 1.0f / LightProxy.GetRadius()));

		SetShaderValue(ShaderRHI, ShadowFadeFraction, ShadowInfo->FadeAlphas[ViewIndex]);
		SetShaderValue(ShaderRHI, ShadowSharpen, LightProxy.GetShadowSharpen() * 7.0f + 1.0f);
		//Near is always 1? // TODO: validate
		float Near = 1;
		float Far = LightProxy.GetRadius();
		Vector2 param = Vector2(Far / (Far - Near), -Near * Far / (Far - Near));
		Vector2 projParam = Vector2(1.0f / param.Y, param.X / param.Y);
		SetShaderValue(ShaderRHI, PointLightDepthBiasAndProjParameters, Vector4(ShadowInfo->GetShaderDepthBias(), 0.0f, projParam.X, projParam.Y));

		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
		{
			const PooledRenderTarget* PooledRT = nullptr;// GetSubsufaceProfileTexture_RT((FRHICommandListImmediate&)RHICmdList);

			if (!PooledRT)
			{
				// no subsurface profile was used yet
				PooledRT = GSystemTextures.BlackDummy.Get();
			}

			ID3D11ShaderResourceView* const Item = PooledRT->ShaderResourceTexture->GetShaderResourceView();

			SetTextureParameter(ShaderRHI, TransmissionProfilesTexture, Item);
		}

		FScene* Scene = nullptr;

		if (View.Family->Scene)
		{
			Scene = View.Family->Scene;
		}

		SetSamplerParameter(ShaderRHI, ShadowDepthTextureSampler, TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI());

		auto DeferredLightParameter = GetUniformBufferParameter<FDeferredLightUniformStruct>();

		if (DeferredLightParameter.IsBound())
		{
			SetDeferredLightParameters(ShaderRHI, DeferredLightParameter, &ShadowInfo->GetLightSceneInfo(), View);
		}
	}

private:
	FSceneTextureShaderParameters SceneTextureParameters;
	FOnePassPointShadowProjectionShaderParameters OnePassShadowParameters;
	FShaderResourceParameter ShadowDepthTextureSampler;
	FShaderParameter LightPosition;
	FShaderParameter ShadowFadeFraction;
	FShaderParameter ShadowSharpen;
	FShaderParameter PointLightDepthBiasAndProjParameters;
	FShaderResourceParameter TransmissionProfilesTexture;
};