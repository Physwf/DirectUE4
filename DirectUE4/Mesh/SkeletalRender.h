
#pragma once

#include "UnrealMathFPU.h"

#include <vector>

class USkinnedMeshComponent;
class FSkeletalMeshRenderData;

/**
* Utility function that fills in the array of ref-pose to local-space matrices using
* the mesh component's updated space bases
* @param	ReferenceToLocal - matrices to update
* @param	SkeletalMeshComponent - mesh primitive with updated bone matrices
* @param	InSkeletalMeshRenderData - resource for which to compute RefToLocal matrices
* @param	LODIndex - each LOD has its own mapping of bones to update
* @param	ExtraRequiredBoneIndices - any extra bones apart from those active in the LOD that we'd like to update
*/
void UpdateRefToLocalMatrices(std::vector<FMatrix>& ReferenceToLocal, const USkinnedMeshComponent* InMeshComponent, const FSkeletalMeshRenderData* InSkeletalMeshRenderData, int32 LODIndex, const std::vector<FBoneIndexType>* ExtraRequiredBoneIndices = NULL);

/**
* Utility function that fills in the array of ref-pose to local-space matrices using
* the mesh component's updated previous space bases
* @param	ReferenceToLocal - matrices to update
* @param	SkeletalMeshComponent - mesh primitive with updated bone matrices
* @param	InSkeletalMeshRenderData - resource for which to compute RefToLocal matrices
* @param	LODIndex - each LOD has its own mapping of bones to update
* @param	ExtraRequiredBoneIndices - any extra bones apart from those active in the LOD that we'd like to update
*/
void UpdatePreviousRefToLocalMatrices(std::vector<FMatrix>& ReferenceToLocal, const USkinnedMeshComponent* InMeshComponent, const FSkeletalMeshRenderData* InSkeletalMeshRenderData, int32 LODIndex, const std::vector<FBoneIndexType>* ExtraRequiredBoneIndices = NULL);


extern const VectorRegister		VECTOR_0001;

/**
* Apply scale/bias to packed normal byte values and store result in register
* Only first 3 components are copied. W component is always 0
*
* @param PackedNormal - source vector packed with byte components
* @return vector register with unpacked float values
*/
static FORCEINLINE VectorRegister Unpack3(const uint32 *PackedNormal)
{
	return VectorMultiply(VectorLoadSignedByte4(PackedNormal), VectorSetFloat3(1.0f / 127.0f, 1.0f / 127.0f, 1.0f / 127.0f));
}

/**
* Apply scale/bias to float register values and store results in memory as packed byte values
* Only first 3 components are copied. W component is always 0
*
* @param Normal - source vector register with floats
* @param PackedNormal - destination vector packed with byte components
*/
static FORCEINLINE void Pack3(VectorRegister Normal, uint32 *PackedNormal)
{
	Normal = VectorMultiply(Normal, VectorSetFloat3(127.0f, 127.0f, 127.0f));
	VectorStoreSignedByte4(Normal, PackedNormal);
}

/**
* Apply scale/bias to packed normal byte values and store result in register
* All 4 components are copied.
*
* @param PackedNormal - source vector packed with byte components
* @return vector register with unpacked float values
*/
static FORCEINLINE VectorRegister Unpack4(const uint32 *PackedNormal)
{
	return VectorMultiply(VectorLoadSignedByte4(PackedNormal), VectorSetFloat1(1.0f / 127.0f));
}

/**
* Apply scale/bias to float register values and store results in memory as packed byte values
* All 4 components are copied.
*
* @param Normal - source vector register with floats
* @param PackedNormal - destination vector packed with byte components
*/
static FORCEINLINE void Pack4(VectorRegister Normal, uint32 *PackedNormal)
{
	Normal = VectorMultiply(Normal, VectorSetFloat1(127.0f));
	VectorStoreSignedByte4(Normal, PackedNormal);
}