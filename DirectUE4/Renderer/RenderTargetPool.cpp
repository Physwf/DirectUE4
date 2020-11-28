#include "RenderTargetPool.h"

#define SafeRelease(Resource) if(Resource){ Resource->Release(); Resource = NULL;}

uint32 PooledRenderTarget::AddRef() const
{
	if (!bSnapshot)
	{
		return uint32(++NumRefs);
	}
	assert(NumRefs == 1);
	return 1;
}

uint32 PooledRenderTarget::Release()
{
	if (!bSnapshot)
	{
		uint32 Refs = uint32(--NumRefs);
		if (Refs == 0)
		{
			delete this;
		}
		else if (Refs == 1 && Pool && IsTransient())
		{
			// Discard the resource
// 			check(GetRenderTargetItem().TargetableTexture != nullptr);
// 			if (GetRenderTargetItem().TargetableTexture)
// 			{
// 				RHIDiscardTransientResource(GetRenderTargetItem().TargetableTexture);
// 			}
// 			FrameNumberLastDiscard = GFrameNumberRenderThread;
		}
		return Refs;
	}
	assert(NumRefs == 1);
	return 1;
}

uint32 PooledRenderTarget::GetRefCount() const
{
	return uint32(NumRefs);
}

bool PooledRenderTarget::IsFree() const
{
	uint32 RefCount = GetRefCount();
	assert(RefCount >= 1);

	// If the only reference to the pooled render target is from the pool, then it's unused.
	return !bSnapshot && RefCount == 1;
}

const PooledRenderTargetDesc& PooledRenderTarget::GetDesc() const
{
	return Desc;
}

RenderTargetPool GRenderTargetPool;

RenderTargetPool::RenderTargetPool()
{

}

bool RenderTargetPool::FindFreeElement(const PooledRenderTargetDesc& InputDesc, ComPtr<PooledRenderTarget> &Out, const wchar_t* DebugName)
{
	if (!InputDesc.IsValid())
	{
		// no need to do anything
		return true;
	}

	assert(InputDesc.NumMips > 0);

	// Make sure if requesting a depth format that the clear value is correct
	//ensure(!IsDepthOrStencilFormat(InputDesc.Format) || (InputDesc.ClearValue.ColorBinding == EClearBinding::ENoneBound || InputDesc.ClearValue.ColorBinding == EClearBinding::EDepthStencilBound));

	// If we're doing aliasing, we may need to override Transient flags, depending on the input format and mode
	// 	FPooledRenderTargetDesc ModifiedDesc;
	// 	bool bMakeTransient = DoesTargetNeedTransienceOverride(InputDesc, TransienceHint);
	// 	if (bMakeTransient)
	// 	{
	// 		ModifiedDesc = InputDesc;
	// 		ModifiedDesc.Flags |= TexCreate_Transient;
	// 	}
	// 
	// 	// Override the descriptor if necessary
	// 	const FPooledRenderTargetDesc& Desc = bMakeTransient ? ModifiedDesc : InputDesc;
	// if we can keep the current one, do that

	const PooledRenderTargetDesc& Desc = InputDesc;
	if (Out.Get())
	{
		PooledRenderTarget* Current = (PooledRenderTarget*)Out.Get();

		assert(!Current->IsSnapshot());

		const bool bExactMatch = true;

		if (Out->GetDesc().Compare(Desc, bExactMatch))
		{
			// we can reuse the same, but the debug name might have changed
// 			Current->Desc.DebugName = InDebugName;
// 			RHIBindDebugLabelName(Current->GetRenderTargetItem().TargetableTexture, InDebugName);
			assert(!Out->IsFree());
			// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			// 			ClobberAllocatedRenderTarget(RHICmdList, Desc, Out);
			// #endif
			return true;
		}
		else
		{
			// release old reference, it might free a RT we can use
			Out.Reset();

			if (Current->IsFree())
			{
				//AllocationLevelInKB -= ComputeSizeInKB(*Current);

				int32 Index = FindIndex(Current);

				assert(Index >= 0);

				// we don't use Remove() to not shuffle around the elements for better transparency on RenderTargetPoolEvents
				PooledRenderTargets[Index].Reset();

				//VerifyAllocationLevel();
			}
		}
	}

	//int32 AliasingMode = CVarRtPoolTransientMode.GetValueOnRenderThread();
	PooledRenderTarget* Found = 0;
	uint32 FoundIndex = -1;
	bool bReusingExistingTarget = false;
	// try to find a suitable element in the pool
	{
		//don't spend time doing 2 passes if the platform doesn't support fastvram
		uint32 PassCount = 1;
// 		if (AliasingMode == 0)
// 		{
// 			if ((Desc.Flags & TexCreate_FastVRAM) && FPlatformMemory::SupportsFastVRAMMemory())
// 			{
// 				PassCount = 2;
// 			}
// 		}

		//bool bAllowMultipleDiscards = (CVarAllowMultipleAliasingDiscardsPerFrame.GetValueOnRenderThread() != 0);
		// first we try exact, if that fails we try without TexCreate_FastVRAM
		// (easily we can run out of VRam, if this search becomes a performance problem we can optimize or we should use less TexCreate_FastVRAM)
		for (uint32 Pass = 0; Pass < PassCount; ++Pass)
		{
			bool bExactMatch = (Pass == 0); //-V547

			for (uint32 i = 0, Num = (uint32)PooledRenderTargets.size(); i < Num; ++i)
			{
				PooledRenderTarget* Element = PooledRenderTargets[i].Get();

				if (Element && Element->IsFree() && Element->GetDesc().Compare(Desc, bExactMatch))
				{
// 					if ((Desc.Flags & TexCreate_Transient) && bAllowMultipleDiscards == false && Element->HasBeenDiscardedThisFrame())
// 					{
// 						// We can't re-use transient resources if they've already been discarded this frame
// 						continue;
// 					}
					assert(!Element->IsSnapshot());
					Found = Element;
					FoundIndex = i;
					bReusingExistingTarget = true;
					goto Done;
				}
			}
		}
	}
Done:

	if (!Found)
	{
		//UE_LOG(LogRenderTargetPool, Display, TEXT("%d MB, NewRT %s %s"), (AllocationLevelInKB + 1023) / 1024, *Desc.GenerateInfoString(), InDebugName);

		// not found in the pool, create a new element
		Found = new PooledRenderTarget(Desc, this);

		PooledRenderTargets.push_back(Found);

		// TexCreate_UAV should be used on Desc.TargetableFlags
		assert(!(Desc.Flags & TexCreate_UAV));
		// TexCreate_FastVRAM should be used on Desc.Flags
		assert(!(Desc.TargetableFlags & TexCreate_FastVRAM));

		if (Desc.TargetableFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable | TexCreate_UAV))
		{
			if (Desc.Is2DTexture())
			{
				if (!Desc.IsArray())
				{
					RHICreateTargetableShaderResource2D(
						Desc.Extent.X,
						Desc.Extent.Y,
						Desc.Format,
						Desc.NumMips,
						Desc.Flags,
						Desc.TargetableFlags,
						Desc.bForceSeparateTargetAndShaderResource,
						Desc.ClearValue,
						Found->TargetableTexture,
						Found->ShaderResourceTexture,
						Desc.NumSamples
					);
				}
				else
				{
					RHICreateTargetableShaderResource2DArray(
						Desc.Extent.X,
						Desc.Extent.Y,
						Desc.ArraySize,
						Desc.Format,
						Desc.NumMips,
						Desc.Flags,
						Desc.TargetableFlags,
						Desc.ClearValue,
						Found->TargetableTexture,
						Found->ShaderResourceTexture,
						Desc.NumSamples
					);
				}

// 				if (GSupportsRenderTargetWriteMask && Desc.bCreateRenderTargetWriteMask)
// 				{
// 					Found->RenderTargetItem.RTWriteMaskDataBufferRHI = RHICreateRTWriteMaskBuffer((FTexture2DRHIRef&)Found->RenderTargetItem.TargetableTexture);
// 					Found->RenderTargetItem.RTWriteMaskBufferRHI_SRV = RHICreateShaderResourceView(Found->RenderTargetItem.RTWriteMaskDataBufferRHI);
// 				}

				if (Desc.NumMips > 1)
				{
					Found->MipSRVs.resize(Desc.NumMips);
					for (uint16 i = 0; i < Desc.NumMips; i++)
					{
						Found->MipSRVs[i] = RHICreateShaderResourceView(Found->ShaderResourceTexture,i); 
					}
				}
			}
			else if (Desc.Is3DTexture())
			{
				// 				Found->RenderTargetItem.ShaderResourceTexture = RHICreateTexture3D(
				// 					Desc.Extent.X,
				// 					Desc.Extent.Y,
				// 					Desc.Depth,
				// 					Desc.Format,
				// 					Desc.NumMips,
				// 					Desc.Flags | Desc.TargetableFlags,
				// 					CreateInfo);
				// 
				// 				// similar to RHICreateTargetableShaderResource2D
				// 				Found->RenderTargetItem.TargetableTexture = Found->RenderTargetItem.ShaderResourceTexture;
			}
			else
			{
				assert(Desc.IsCubemap());
				if (Desc.IsArray())
				{
					RHICreateTargetableShaderResourceCubeArray(
						Desc.Extent.X,
						Desc.ArraySize,
						Desc.Format,
						Desc.NumMips,
						Desc.Flags,
						Desc.TargetableFlags,
						false,
						Desc.ClearValue,
						Found->TargetableTexture,
						Found->ShaderResourceTexture
					);
				}
				else
				{
					RHICreateTargetableShaderResourceCube(
						Desc.Extent.X,
						Desc.Format,
						Desc.NumMips,
						Desc.Flags,
						Desc.TargetableFlags,
						false,
						Desc.ClearValue,
						Found->TargetableTexture,
						Found->ShaderResourceTexture
					);

					if (Desc.NumMips > 1)
					{
						Found->MipSRVs.resize(Desc.NumMips);
						for (uint16 i = 0; i < Desc.NumMips; i++)
						{
							Found->MipSRVs[i] = RHICreateShaderResourceView(Found->ShaderResourceTexture, i);
						}
					}
				}

			}

			//RHIBindDebugLabelName(Found->RenderTargetItem.TargetableTexture, InDebugName);
		}
		else
		{
			if (Desc.Is2DTexture())
			{
				// this is useful to get a CPU lockable texture through the same interface
				Found->ShaderResourceTexture = RHICreateTexture2D(
					Desc.Extent.X,
					Desc.Extent.Y,
					Desc.Format,
					Desc.NumMips,
					Desc.NumSamples,
					Desc.Flags,
					Desc.ClearValue);
			}
			else if (Desc.Is3DTexture())
			{
				// 				Found->RenderTargetItem.ShaderResourceTexture = RHICreateTexture3D(
				// 					Desc.Extent.X,
				// 					Desc.Extent.Y,
				// 					Desc.Depth,
				// 					Desc.Format,
				// 					Desc.NumMips,
				// 					Desc.Flags,
				// 					CreateInfo);
			}
			else
			{
				// 				check(Desc.IsCubemap());
				// 				if (Desc.IsArray())
				// 				{
				// 					FTextureCubeRHIRef CubeTexture = RHICreateTextureCubeArray(Desc.Extent.X, Desc.ArraySize, Desc.Format, Desc.NumMips, Desc.Flags | Desc.TargetableFlags | TexCreate_ShaderResource, CreateInfo);
				// 					Found->RenderTargetItem.TargetableTexture = Found->RenderTargetItem.ShaderResourceTexture = CubeTexture;
				// 				}
				// 				else
				// 				{
				// 					FTextureCubeRHIRef CubeTexture = RHICreateTextureCube(Desc.Extent.X, Desc.Format, Desc.NumMips, Desc.Flags | Desc.TargetableFlags | TexCreate_ShaderResource, CreateInfo);
				// 					Found->RenderTargetItem.TargetableTexture = Found->RenderTargetItem.ShaderResourceTexture = CubeTexture;
				// 				}
			}

			//RHIBindDebugLabelName(Found->RenderTargetItem.ShaderResourceTexture, InDebugName);
		}

		if (Desc.TargetableFlags & TexCreate_UAV)
		{
			// The render target desc is invalid if a UAV is requested with an RHI that doesn't support the high-end feature level.
			//check(GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5);
			Found->MipUAVs.reserve(Desc.NumMips);
			for (uint32 MipLevel = 0; MipLevel < Desc.NumMips; MipLevel++)
			{
				//Found->MipUAVs.Add(RHICreateUnorderedAccessView(Found->RenderTargetItem.TargetableTexture, MipLevel));
			}

			//Found->UAV = Found->MipUAVs[0];
		}

		//AllocationLevelInKB += ComputeSizeInKB(*Found);
		//VerifyAllocationLevel();

		FoundIndex = PooledRenderTargets.size() - 1;
	}

	Out = Found;

	return false;
}

int32 RenderTargetPool::FindIndex(PooledRenderTarget* In) const
{
	if (In)
	{
		for (uint32 i = 0, Num = (uint32)PooledRenderTargets.size(); i < Num; ++i)
		{
			const PooledRenderTarget* Element = PooledRenderTargets[i].Get();

			if (Element == In)
			{
				assert(!Element->IsSnapshot());
				return i;
			}
		}
	}

	// not found
	return -1;
}

