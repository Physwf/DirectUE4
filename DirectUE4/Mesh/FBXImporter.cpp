#include "FBXImporter.h"
#include "log.h"
#include "SkeletalMesh.h"
#include "StaticMesh.h"
#include "Skeleton.h"
#include "mikktspace.h"
#include "UnrealTemplates.h"

#include <algorithm>

// Get the geometry deformation local to a node. It is never inherited by the
// children.
FbxAMatrix GetGeometry(FbxNode* pNode)
{
	FbxVector4 lT, lR, lS;
	FbxAMatrix lGeometry;

	lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	lGeometry.SetT(lT);
	lGeometry.SetR(lR);
	lGeometry.SetS(lS);

	return lGeometry;
}

void FillFbxArray(FbxNode* pNode, std::vector<FbxNode*>& pOutMeshArray)
{
	if (pNode->GetMesh())
	{
		pOutMeshArray.push_back(pNode);
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i)
	{
		FillFbxArray(pNode->GetChild(i), pOutMeshArray);
	}
}

FbxAMatrix ComputeTotalMatrix(FbxScene* pScence, FbxNode* pNode, bool bTransformVertexToAbsolute, bool bBakePivotInVertex)
{
	FbxAMatrix Geometry;
	FbxVector4 Translation, Rotation, Scaling;
	Translation = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	Rotation = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	Scaling = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
	Geometry.SetT(Translation);
	Geometry.SetR(Rotation);
	Geometry.SetS(Scaling);

	FbxAMatrix& GlobalTransform = pScence->GetAnimationEvaluator()->GetNodeGlobalTransform(pNode);
	if (!bTransformVertexToAbsolute)
	{
		if (bBakePivotInVertex)
		{
			FbxAMatrix PivotGeometry;
			FbxVector4 RotationPivot = pNode->GetRotationPivot(FbxNode::eSourcePivot);
			FbxVector4 FullPivot;
			FullPivot[0] = -RotationPivot[0];
			FullPivot[1] = -RotationPivot[1];
			FullPivot[2] = -RotationPivot[2];
			PivotGeometry.SetT(FullPivot);
			Geometry = Geometry * PivotGeometry;
		}
		else
		{
			Geometry.SetIdentity();
		}
	}

	FbxAMatrix TotalMatrix = bTransformVertexToAbsolute ? GlobalTransform * Geometry : Geometry;

	return TotalMatrix;
}
FbxAMatrix JointPostConversionMatrix;
void SetJointPostConversionMatrix(FbxAMatrix ConversionMatrix)
{
	JointPostConversionMatrix = ConversionMatrix;
}
const FbxAMatrix & GetJointPostConversionMatrix()
{
	return JointPostConversionMatrix;
}

FVector ConvertPos(FbxVector4 pPos)
{
	FVector Result;
	Result.X = (float)pPos[0];
	Result.Y = -(float)pPos[1];
	Result.Z = (float)pPos[2];
	return Result;
}
FVector ConvertDir(FbxVector4 Vec)
{
	FVector Result;
	Result.X = (float)Vec[0];
	Result.Y = -(float)Vec[1];
	Result.Z = (float)Vec[2];
	return Result;
}
float ConvertDist(FbxDouble Distance)
{
	float Out;
	Out = (float)Distance;
	return Out;
}
FQuat ConvertRotToQuat(FbxQuaternion Quaternion)
{
	FQuat UnrealQuat;
	UnrealQuat.X = (float)Quaternion[0];
	UnrealQuat.Y = -(float)Quaternion[1];
	UnrealQuat.Z = (float)Quaternion[2];
	UnrealQuat.W = -(float)Quaternion[3];

	return UnrealQuat;
}
FVector ConvertScale(FbxDouble3 V)
{
	FVector Out;
	Out[0] = (float)V[0];
	Out[1] = (float)V[1];
	Out[2] = (float)V[2];
	return Out;
}
// Scale all the elements of a matrix.
void MatrixScale(FbxAMatrix& pMatrix, double pValue)
{
	int32 i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pMatrix[i][j] *= pValue;
		}
	}
}
// Add a value to all the elements in the diagonal of the matrix.
void MatrixAddToDiagonal(FbxAMatrix& pMatrix, double pValue)
{
	pMatrix[0][0] += pValue;
	pMatrix[1][1] += pValue;
	pMatrix[2][2] += pValue;
	pMatrix[3][3] += pValue;
}
// Sum two matrices element by element.
void MatrixAdd(FbxAMatrix& pDstMatrix, FbxAMatrix& pSrcMatrix)
{
	int32 i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pDstMatrix[i][j] += pSrcMatrix[i][j];
		}
	}
}
bool IsUnrealBone(FbxNode* Link)
{
	FbxNodeAttribute* Attr = Link->GetNodeAttribute();
	if (Attr)
	{
		FbxNodeAttribute::EType AttrType = Attr->GetAttributeType();
		if (AttrType == FbxNodeAttribute::eSkeleton ||
			AttrType == FbxNodeAttribute::eMesh ||
			AttrType == FbxNodeAttribute::eNull)
		{
			return true;
		}
	}

	return false;
}

void FSkeletalMeshImportData::CopyLODImportData(std::vector<FVector>& LODPoints, std::vector<FMeshWedge>& LODWedges, std::vector<FMeshFace>& LODFaces, std::vector<FVertInfluence>& LODInfluences, std::vector<int32>& LODPointToRawMap) const
{
	// Copy vertex data.
	LODPoints.clear();
	LODPoints.resize(Points.size());
	for (uint32 p = 0; p < Points.size(); p++)
	{
		LODPoints[p] = Points[p];
	}

	// Copy wedge information to static LOD level.
	LODWedges.clear();
	LODWedges.resize(Wedges.size());
	for (uint32 w = 0; w < Wedges.size(); w++)
	{
		LODWedges[w].iVertex = Wedges[w].VertexIndex;
		// Copy all texture coordinates
		memcpy(LODWedges[w].UVs, Wedges[w].UVs, sizeof(Vector2) * 4/*MAX_TEXCOORDS*/);
		LODWedges[w].Color = Wedges[w].Color;

	}

	// Copy triangle/ face data to static LOD level.
	LODFaces.clear();
	LODFaces.resize(Faces.size());
	for (uint32 f = 0; f < Faces.size(); f++)
	{
		FMeshFace Face;
		Face.iWedge[0] = Faces[f].WedgeIndex[0];
		Face.iWedge[1] = Faces[f].WedgeIndex[1];
		Face.iWedge[2] = Faces[f].WedgeIndex[2];
		Face.MeshMaterialIndex = Faces[f].MatIndex;

		Face.TangentX[0] = Faces[f].TangentX[0];
		Face.TangentX[1] = Faces[f].TangentX[1];
		Face.TangentX[2] = Faces[f].TangentX[2];

		Face.TangentY[0] = Faces[f].TangentY[0];
		Face.TangentY[1] = Faces[f].TangentY[1];
		Face.TangentY[2] = Faces[f].TangentY[2];

		Face.TangentZ[0] = Faces[f].TangentZ[0];
		Face.TangentZ[1] = Faces[f].TangentZ[1];
		Face.TangentZ[2] = Faces[f].TangentZ[2];

		Face.SmoothingGroups = Faces[f].SmoothingGroups;

		LODFaces[f] = Face;
	}

	// Copy weights/ influences to static LOD level.
	LODInfluences.clear();
	LODInfluences.resize(Influences.size());
	for (uint32 i = 0; i < Influences.size(); i++)
	{
		LODInfluences[i].Weight = Influences[i].Weight;
		LODInfluences[i].VertIndex = Influences[i].VertexIndex;
		LODInfluences[i].BoneIndex = Influences[i].BoneIndex;
	}

	// Copy mapping
	LODPointToRawMap = PointToRawMap;
}

bool ProcessImportMeshSkeleton(const USkeleton* SkeletonAsset, FReferenceSkeleton& RefSkeleton, int32& SkeletalDepth, FSkeletalMeshImportData& ImportData)
{
	std::vector<VBone>&	RefBonesBinary = ImportData.RefBonesBinary;

	// Setup skeletal hierarchy + names structure.
	RefSkeleton.Empty();

	FReferenceSkeletonModifier RefSkelModifier(RefSkeleton, SkeletonAsset);

	// Digest bones to the serializable format.
	for (uint32 b = 0; b < RefBonesBinary.size(); b++)
	{
		const VBone & BinaryBone = RefBonesBinary[b];
		const std::string BoneName = BinaryBone.Name;
		const FMeshBoneInfo BoneInfo(BoneName, BinaryBone.Name, BinaryBone.ParentIndex);
		const FTransform BoneTransform(BinaryBone.BonePos.Transform);

		if (RefSkeleton.FindRawBoneIndex(BoneInfo.Name) != -1)
		{
			X_LOG("Skeleton has non-unique bone names.\nBone named '%s' encountered more than once.", BoneInfo.Name.c_str());
			return false;
		}

		RefSkelModifier.Add(BoneInfo, BoneTransform);
	}

	// Add hierarchy index to each bone and detect max depth.
	SkeletalDepth = 0;

	std::vector<int32> SkeletalDepths;
	SkeletalDepths.clear();
	//SkeletalDepths.AddZeroed(RefBonesBinary.Num());
	SkeletalDepths.resize(RefBonesBinary.size());
	for (int32 b = 0; b < RefSkeleton.GetRawBoneNum(); b++)
	{
		int32 Parent = RefSkeleton.GetRawParentIndex(b);
		int32 Depth = 1;

		SkeletalDepths[b] = 1;
		if (Parent != -1)
		{
			Depth += SkeletalDepths[Parent];
		}
		if (SkeletalDepth < Depth)
		{
			SkeletalDepth = Depth;
		}
		SkeletalDepths[b] = Depth;
	}

	return true;
}


struct FBXUVs
{
	explicit FBXUVs(FbxMesh* Mesh)
		:UniqueUVCount(0)
	{
		assert(Mesh != NULL);

		int LayerCount = Mesh->GetLayerCount();
		if (LayerCount > 0)
		{
			int UVLayerIndex;
			for (UVLayerIndex = 0; UVLayerIndex < LayerCount; UVLayerIndex++)
			{
				FbxLayer* lLayer = Mesh->GetLayer(UVLayerIndex);
				int UVSetCount = lLayer->GetUVSetCount();
				if (UVSetCount)
				{
					FbxArray<FbxLayerElementUV const*> EleUVs = lLayer->GetUVSets();
					for (int UVIndex = 0; UVIndex < UVSetCount; ++UVIndex)
					{
						FbxLayerElementUV const* ElementUV = EleUVs[UVIndex];
						if (ElementUV)
						{
							const char* UVSetName = ElementUV->GetName();
							std::string LocalUVSetName = UVSetName;
							if (LocalUVSetName.size() == 0)
							{
								LocalUVSetName = std::string("UVmap_") + std::to_string(UVLayerIndex);
							}
							UVSets.push_back(LocalUVSetName);
						}
					}
				}
			}
		}


		if (UVSets.size())
		{
			for (int ChannelNumIdx = 0; ChannelNumIdx < 4; ChannelNumIdx++)
			{
				std::string ChannelName = std::string("UVChannel_") + std::to_string(ChannelNumIdx + 1);
				auto it = std::find(UVSets.begin(), UVSets.end(), ChannelName);
				int SetIdx = -1;
				if (it != UVSets.end())
				{
					SetIdx = it - UVSets.begin();
				}

				if (SetIdx != -1 && SetIdx != ChannelNumIdx)
				{
					for (int ArrSize = UVSets.size(); ArrSize < ChannelNumIdx + 1; ArrSize++)
					{
						UVSets.push_back(std::string(""));
					}
					std::swap(UVSets[SetIdx], UVSets[ChannelNumIdx]);
				}
			}
		}
	}

	void Phase2(FbxMesh* Mesh)
	{
		UniqueUVCount = UVSets.size();
		if (UniqueUVCount > 0)
		{
			LayerElementUV.resize(UniqueUVCount);
			UVReferenceMode.resize(UniqueUVCount);
			UVMappingMode.resize(UniqueUVCount);
		}
		for (int32 UVIndex = 0; UVIndex < UniqueUVCount; UVIndex++)
		{
			LayerElementUV[UVIndex] = NULL;
			for (int UVLayerIndex = 0, LayerCount = Mesh->GetLayerCount(); UVLayerIndex < LayerCount; UVLayerIndex++)
			{
				FbxLayer* lLayer = Mesh->GetLayer(UVLayerIndex);
				int UVSetCount = lLayer->GetUVSetCount();
				if (UVSetCount > 0)
				{
					FbxArray<FbxLayerElementUV const*> EleUVs = lLayer->GetUVSets();
					for (int FbxUVIndex = 0; FbxUVIndex < UVSetCount; FbxUVIndex++)
					{
						FbxLayerElementUV const* ElementUV = EleUVs[FbxUVIndex];
						if (ElementUV)
						{
							const char* UVSetName = ElementUV->GetName();
							std::string LocalUVSetName = UVSetName;
							if (LocalUVSetName.size() == 0)
							{
								LocalUVSetName = std::string("UVmap_") + std::to_string(UVLayerIndex);
							}
							if (LocalUVSetName == UVSets[UVIndex])
							{
								LayerElementUV[UVIndex] = ElementUV;
								UVReferenceMode[UVIndex] = ElementUV->GetReferenceMode();
								UVMappingMode[UVIndex] = ElementUV->GetMappingMode();
								break;
							}
						}
					}
				}
			}
		}
		UniqueUVCount = (std::min)(UniqueUVCount, 8);
	}

	int FindLightUVIndex() const
	{
		for (int UVSetIdx = 0; UVSetIdx < (int)UVSets.size(); UVSetIdx++)
		{
			if (UVSets[UVSetIdx] == "LightMapUV")
			{
				return UVSetIdx;
			}
		}

		return -1;
	}

	int ComputeUVIndex(int UVLayerIndex, int lControlPointIndex, int FaceCornerIndex) const
	{
		int UVMapIndex = (UVMappingMode[UVLayerIndex] == FbxLayerElement::eByControlPoint) ? lControlPointIndex : FaceCornerIndex;
		int Ret;

		if (UVReferenceMode[UVLayerIndex] == FbxLayerElement::eDirect)
		{
			Ret = UVMapIndex;
		}
		else
		{
			FbxLayerElementArrayTemplate<int>& Array = LayerElementUV[UVLayerIndex]->GetIndexArray();
			Ret = Array.GetAt(UVMapIndex);
		}

		return Ret;
	}

	void CleanUp()
	{
		LayerElementUV.clear();
		UVReferenceMode.clear();
		UVMappingMode.clear();
	}

	std::vector<std::string> UVSets;
	std::vector<FbxLayerElementUV const*> LayerElementUV;
	std::vector<FbxLayerElement::EReferenceMode> UVReferenceMode;
	std::vector<FbxLayerElement::EMappingMode> UVMappingMode;
	int UniqueUVCount;
};


UStaticMesh* FBXImporter::ImportStaticMesh(class AActor* InOwner, const char* pFileName)
{
	FbxManager* lFbxManager = FbxManager::Create();

	FbxIOSettings* lIOSetting = FbxIOSettings::Create(lFbxManager, IOSROOT);
	lFbxManager->SetIOSettings(lIOSetting);

	FbxImporter* lImporter = FbxImporter::Create(lFbxManager, "MeshImporter");
	if (!lImporter->Initialize(pFileName, -1, lFbxManager->GetIOSettings()))
	{
		X_LOG("Call to FbxImporter::Initialize() failed.\n");
		X_LOG("Error returned: %s \n\n", lImporter->GetStatus().GetErrorString());
		return NULL;
	}

	UStaticMesh* Mesh = new UStaticMesh(InOwner);
	MeshDescription& MD = Mesh->GetMeshDescription();

	FbxScene* lScene = FbxScene::Create(lFbxManager, "Mesh");
	lImporter->Import(lScene);
	lImporter->Destroy();

	FbxAxisSystem::ECoordSystem CoordSystem = FbxAxisSystem::eRightHanded;
	FbxAxisSystem::EUpVector UpVector = FbxAxisSystem::eZAxis;
	FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector) - FbxAxisSystem::eParityOdd;

	FbxAxisSystem UnrealImportAxis(UpVector, FrontVector, CoordSystem);
	FbxAxisSystem SourceSetup = lScene->GetGlobalSettings().GetAxisSystem();

	if (SourceSetup != UnrealImportAxis)
	{
		FbxRootNodeUtility::RemoveAllFbxRoots(lScene);
		UnrealImportAxis.ConvertScene(lScene);
		// 		FbxAMatrix JointOrientationMatrix;
		// 		JointOrientationMatrix.SetIdentity();
		// 		if (GetImportOptions()->bForceFrontXAxis)
		// 		{
		// 			JointOrientationMatrix.SetR(FbxVector4(-90.0, -90.0, 0.0));
		// 		}
		//FFbxDataConverter::SetJointPostConversionMatrix(JointOrientationMatrix);
	}

	std::vector<FbxNode*> lOutMeshArray;
	FillFbxArray(lScene->GetRootNode(), lOutMeshArray);

	FbxGeometryConverter* GeometryConverter = new FbxGeometryConverter(lFbxManager);

	std::vector<FbxMaterial> MeshMaterials;

	for (int Index = 0; Index < (int)lOutMeshArray.size(); ++Index)
	{
		FbxNode* lNode = lOutMeshArray[Index];

		int MaterialCount = lNode->GetMaterialCount();
		int MaterialIndexOffset = (int)MeshMaterials.size();
		for (int MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			MeshMaterials.push_back(FbxMaterial());
			FbxMaterial* NewMaterial = &MeshMaterials.back();
			FbxSurfaceMaterial* fbxMaterial = lNode->GetMaterial(MaterialIndex);
			NewMaterial->fbxMaterial = fbxMaterial;
		}


		if (lNode->GetMesh())
		{
			FbxMesh* lMesh = lNode->GetMesh();

			lMesh->RemoveBadPolygons();

			if (!lMesh->IsTriangleMesh())
			{
				FbxNodeAttribute* ConvertedNode = GeometryConverter->Triangulate(lMesh, true);

				if (ConvertedNode != NULL && ConvertedNode->GetAttributeType() == FbxNodeAttribute::eMesh)
				{
					lMesh = ConvertedNode->GetNode()->GetMesh();
				}
				else
				{
					X_LOG("Cant triangulate!");
				}
			}

			FbxLayer* BaseLayer = lMesh->GetLayer(0);

			FBXUVs fbxUVs(lMesh);

			FbxLayerElementMaterial* LayerElementMaterial = BaseLayer->GetMaterials();
			FbxLayerElement::EMappingMode MaterialMappingMode = LayerElementMaterial ? LayerElementMaterial->GetMappingMode() : FbxLayerElement::eByPolygon;

			fbxUVs.Phase2(lMesh);

			bool bSmootingAvaliables = false;
			FbxLayerElementSmoothing* SmoothingInfo = BaseLayer->GetSmoothing();
			FbxLayerElement::EReferenceMode SmoothingReferenceMode(FbxLayerElement::eDirect);
			FbxLayerElement::EMappingMode SmoothingMappingMode(FbxLayerElement::eByEdge);
			if (SmoothingInfo)
			{
				if (SmoothingInfo->GetMappingMode() == FbxLayerElement::eByPolygon)
				{
					GeometryConverter->ComputeEdgeSmoothingFromPolygonSmoothing(lMesh, 0);
				}

				if (SmoothingInfo->GetMappingMode() == FbxLayerElement::eByEdge)
				{
					bSmootingAvaliables = true;
				}

				SmoothingReferenceMode = SmoothingInfo->GetReferenceMode();
				SmoothingMappingMode = SmoothingInfo->GetMappingMode();
			}

			FbxLayerElementVertexColor* LayerElementVertexColor = BaseLayer->GetVertexColors();
			FbxLayerElement::EReferenceMode VertexColorReferenceMode(FbxLayerElement::eDirect);
			FbxLayerElement::EMappingMode VertexColorMappingMode(FbxLayerElement::eByControlPoint);
			if (LayerElementVertexColor)
			{
				VertexColorReferenceMode = LayerElementVertexColor->GetReferenceMode();
				VertexColorMappingMode = LayerElementVertexColor->GetMappingMode();
			}

			FbxLayerElementNormal* LayerElementNormal = BaseLayer->GetNormals();
			FbxLayerElementTangent* LayerElementTangent = BaseLayer->GetTangents();
			FbxLayerElementBinormal* LayerElementBinormal = BaseLayer->GetBinormals();

			bool bHasNTBInfomation = LayerElementNormal && LayerElementTangent && LayerElementBinormal;

			FbxLayerElement::EReferenceMode NormalReferenceMode(FbxLayerElement::eDirect);
			FbxLayerElement::EMappingMode NormalMappingMode(FbxLayerElement::eByControlPoint);
			if (LayerElementNormal)
			{
				NormalReferenceMode = LayerElementNormal->GetReferenceMode();
				NormalMappingMode = LayerElementNormal->GetMappingMode();
			}

			FbxLayerElement::EReferenceMode TangentReferenceMode(FbxLayerElement::eDirect);
			FbxLayerElement::EMappingMode TangentMappingMode(FbxLayerElement::eByControlPoint);
			if (LayerElementTangent)
			{
				TangentReferenceMode = LayerElementTangent->GetReferenceMode();
				TangentMappingMode = LayerElementTangent->GetMappingMode();
			}

			FbxLayerElement::EReferenceMode BinormalReferenceMode(FbxLayerElement::eDirect);
			FbxLayerElement::EMappingMode BinormalMappingMode(FbxLayerElement::eByControlPoint);
			if (LayerElementBinormal)
			{
				BinormalReferenceMode = LayerElementBinormal->GetReferenceMode();
				BinormalMappingMode = LayerElementBinormal->GetMappingMode();
			}

			FbxAMatrix TotalMatrix;
			FbxAMatrix TotalMatrixForNormal;
			TotalMatrix = ::ComputeTotalMatrix(lScene, lNode, true, false);
			TotalMatrixForNormal = TotalMatrix.Inverse();
			TotalMatrixForNormal = TotalMatrixForNormal.Transpose();
			int PolygonCount = lMesh->GetPolygonCount();

			std::vector<FVector>& VertexPositions = MD.VertexAttributes().GetAttributes<FVector>(MeshAttribute::Vertex::Position);

			std::vector<FVector>& VertexInstanceNormals = MD.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Normal);
			std::vector<FVector>& VertexInstanceTangents = MD.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Tangent);
			std::vector<float>& VertexInstanceBinormalSigns = MD.VertexInstanceAttributes().GetAttributes<float>(MeshAttribute::VertexInstance::BinormalSign);
			std::vector<Vector4>& VertexInstanceColors = MD.VertexInstanceAttributes().GetAttributes<Vector4>(MeshAttribute::VertexInstance::Color);
			std::vector<std::vector<Vector2>>& VertexInstanceUVs = MD.VertexInstanceAttributes().GetAttributesSet<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);

			std::vector<bool>& EdgeHardnesses = MD.EdgeAttributes().GetAttributes<bool>(MeshAttribute::Edge::IsHard);
			std::vector<float>& EdgeCreaseSharpnesses = MD.EdgeAttributes().GetAttributes<float>(MeshAttribute::Edge::CreaseSharpness);

			std::vector<std::string>& PolygonGroupImportedMaterialSlotNames = MD.PolygonGroupAttributes().GetAttributes<std::string>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

			int VertexCount = lMesh->GetControlPointsCount();
			int VertexOffset = MD.Vertices().Num();
			int VertexInstanceOffset = MD.VertexInstances().Num();
			int PolygonOffset = MD.Polygons().Num();

			std::map<int, int> PolygonGroupMapping;

			int ExistingUVCount = 0;
			for (int UVChannelIndex = 0; UVChannelIndex < (int)VertexInstanceUVs.size(); ++UVChannelIndex)
			{
				if (VertexInstanceUVs[UVChannelIndex].size() > 0)
				{
					ExistingUVCount++;
				}
				else
				{
					break;
				}
			}

			int32 NumUVs = (std::max)(fbxUVs.UniqueUVCount, ExistingUVCount);
			NumUVs = (std::min)(8, NumUVs);
			// At least one UV set must exist.  
			NumUVs = (std::max)(1, NumUVs);

			//Make sure all Vertex instance have the correct number of UVs
			VertexInstanceUVs.resize(NumUVs);

			for (auto VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
			{
				int RealVertexIndex = VertexOffset + VertexIndex;
				FbxVector4 FbxPosition = lMesh->GetControlPoints()[VertexIndex];
				FbxPosition = TotalMatrix.MultT(FbxPosition);
				FVector const VectorPositon = ConvertPos(FbxPosition);
				int VertexID = MD.CreateVertex();
				VertexPositions[VertexID] = VectorPositon;
			}

			lMesh->BeginGetMeshEdgeVertices();
			std::map<uint64, int32> RemapEdgeID;
			//Fill the edge array
			int32 FbxEdgeCount = lMesh->GetMeshEdgeCount();
			//RemapEdgeID.Reserve(FbxEdgeCount * 2);
			for (int32 FbxEdgeIndex = 0; FbxEdgeIndex < FbxEdgeCount; ++FbxEdgeIndex)
			{
				int32 EdgeStartVertexIndex = -1;
				int32 EdgeEndVertexIndex = -1;
				lMesh->GetMeshEdgeVertices(FbxEdgeIndex, EdgeStartVertexIndex, EdgeEndVertexIndex);
				int32 EdgeVertexStart(EdgeStartVertexIndex + VertexOffset);
				assert(MD.Vertices().IsValid(EdgeVertexStart));
				int32 EdgeVertexEnd(EdgeEndVertexIndex + VertexOffset);
				assert(MD.Vertices().IsValid(EdgeVertexEnd));
				uint64 CompactedKey = (((uint64)EdgeVertexStart << 32) | ((uint64)EdgeVertexEnd));
				RemapEdgeID.insert(std::make_pair(CompactedKey, FbxEdgeIndex));
				//Add the other edge side
				CompactedKey = ((((uint64)EdgeVertexEnd) << 32) | ((uint64)EdgeVertexStart));
				RemapEdgeID.insert(std::make_pair(CompactedKey, FbxEdgeIndex));
			}
			//Call this after all GetMeshEdgeIndexForPolygon call this is for optimization purpose.
			lMesh->EndGetMeshEdgeVertices();

			lMesh->BeginGetMeshEdgeIndexForPolygon();
			int CurrentVertexInstanceIndex = 0;
			int SkippedVertexInstance = 0;

			for (auto PolygonIndex = 0; PolygonIndex < PolygonCount; ++PolygonIndex)
			{
				int PolygonVertexCount = lMesh->GetPolygonSize(PolygonIndex);
				std::vector<FVector> P;
				P.resize(PolygonVertexCount, FVector());
				for (int32 CornerIndex = 0; CornerIndex < PolygonVertexCount; CornerIndex++)
				{
					const int ControlPointIndex = lMesh->GetPolygonVertex(PolygonIndex, CornerIndex);
					const int VertexID = VertexOffset + ControlPointIndex;
					P[CornerIndex] = VertexPositions[VertexID];
				}

				int RealPolygonIndex = PolygonOffset + PolygonIndex;
				std::vector<int> CornerInstanceIDs;
				CornerInstanceIDs.resize(PolygonVertexCount);
				std::vector<int> CornerVerticesIDs;
				CornerVerticesIDs.resize(PolygonVertexCount);

				for (auto CornerIndex = 0; CornerIndex < PolygonVertexCount; ++CornerIndex, CurrentVertexInstanceIndex++)
				{
					int VertexInstanceIndex = VertexInstanceOffset + CurrentVertexInstanceIndex;
					int RealFbxVertexIndex = SkippedVertexInstance + CurrentVertexInstanceIndex;
					const int VertexInstanceID = VertexInstanceIndex;
					CornerInstanceIDs[CornerIndex] = VertexInstanceID;
					const int ControlPointIndex = lMesh->GetPolygonVertex(PolygonIndex, CornerIndex);
					const int VertexID = VertexOffset + ControlPointIndex;
					const FVector VertexPostion = VertexPositions[VertexID];
					CornerVerticesIDs[CornerIndex] = VertexID;

					int AddedVertexInstanceId = MD.CreateVertexInstance(VertexID);

					for (int UVLayerIndex = 0; UVLayerIndex < fbxUVs.UniqueUVCount; UVLayerIndex++)
					{
						Vector2 FinalUVVector(0.f, 0.f);
						if (fbxUVs.LayerElementUV[UVLayerIndex] != NULL)
						{
							int UVMapIndex = (fbxUVs.UVMappingMode[UVLayerIndex] == FbxLayerElement::eByControlPoint) ? ControlPointIndex : RealFbxVertexIndex;
							int UVIndex = (fbxUVs.UVReferenceMode[UVLayerIndex] == FbxLayerElement::eDirect) ? UVMapIndex : fbxUVs.LayerElementUV[UVLayerIndex]->GetIndexArray().GetAt(UVMapIndex);
							FbxVector2	UVVector = fbxUVs.LayerElementUV[UVLayerIndex]->GetDirectArray().GetAt(UVIndex);
							FinalUVVector.X = static_cast<float>(UVVector[0]);
							FinalUVVector.Y = 1.f - static_cast<float>(UVVector[1]);   //flip the Y of UVs for DirectX
						}
						VertexInstanceUVs[UVLayerIndex][AddedVertexInstanceId] = FinalUVVector;
					}

					if (LayerElementVertexColor)
					{
						int VertexColorMappingIndex = (VertexColorMappingMode == FbxLayerElement::eByControlPoint) ? lMesh->GetPolygonVertex(PolygonIndex, CornerIndex) : RealFbxVertexIndex;
						int VertexColorIndex = (VertexColorReferenceMode == FbxLayerElement::eDirect) ? VertexColorMappingIndex : LayerElementVertexColor->GetIndexArray().GetAt(VertexColorMappingIndex);
						FbxColor VertexColor = LayerElementVertexColor->GetDirectArray().GetAt(VertexColorIndex);
						//todo setcolor
					}

					if (LayerElementNormal)
					{
						int NormalMapIndex = (NormalMappingMode == FbxLayerElement::eByControlPoint) ? ControlPointIndex : RealFbxVertexIndex;
						int NormalValueIndex = (NormalReferenceMode == FbxLayerElement::eDirect) ? NormalMapIndex : LayerElementNormal->GetIndexArray().GetAt(NormalMapIndex);
						FbxVector4 TempValue = LayerElementNormal->GetDirectArray().GetAt(NormalValueIndex);
						TempValue = TotalMatrixForNormal.MultT(TempValue);
						FVector TangentZ = ConvertDir(TempValue);
						VertexInstanceNormals[AddedVertexInstanceId] = TangentZ;

						if (bHasNTBInfomation)
						{
							int TangentMapIndex = (TangentMappingMode == FbxLayerElement::eByControlPoint) ? ControlPointIndex : RealFbxVertexIndex;
							int TangentValueIndex = (TangentReferenceMode == FbxLayerElement::eDirect) ? TangentMapIndex : LayerElementTangent->GetIndexArray().GetAt(TangentMapIndex);

							TempValue = LayerElementTangent->GetDirectArray().GetAt(TangentValueIndex);
							TempValue = TotalMatrixForNormal.MultT(TempValue);
							FVector TangentX = ConvertDir(TempValue);
							VertexInstanceTangents[AddedVertexInstanceId] = TangentX;

							int BinormalMapIndex = (BinormalMappingMode == FbxLayerElement::eByControlPoint) ? ControlPointIndex : RealFbxVertexIndex;
							int BinormalValueIndex = (BinormalReferenceMode == FbxLayerElement::eDirect) ? BinormalMapIndex : LayerElementBinormal->GetIndexArray().GetAt(BinormalMapIndex);

							TempValue = LayerElementBinormal->GetDirectArray().GetAt(BinormalValueIndex);
							TempValue = TotalMatrixForNormal.MultT(TempValue);
							FVector TangentY = -ConvertDir(TempValue);
							VertexInstanceBinormalSigns[AddedVertexInstanceId] = GetBasisDeterminantSign(TangentX.GetSafeNormal(), TangentY.GetSafeNormal(), TangentZ.GetSafeNormal());
						}
					}
				}

				int MaterialIndex = 0;
				if (MaterialCount > 0)
				{
					if (LayerElementMaterial)
					{
						switch (MaterialMappingMode)
						{
						case FbxLayerElement::eAllSame:
							MaterialIndex = LayerElementMaterial->GetIndexArray().GetAt(0);
							break;
						case FbxLayerElement::eByPolygon:
							MaterialIndex = LayerElementMaterial->GetIndexArray().GetAt(PolygonIndex);
							break;
						}
					}
				}

				int RealMaterialIndex = MaterialIndexOffset + MaterialIndex;
				if (PolygonGroupMapping.find(RealMaterialIndex) == PolygonGroupMapping.end())
				{
					std::string ImportedMaterialSlotName = RealMaterialIndex < (int)MeshMaterials.size() ? MeshMaterials[RealMaterialIndex].GetName() : "None";
					int ExistingPolygonGroup = -1;
					for (const int PolygonGroupID : MD.PolygonGroups().GetElementIDs())
					{
						if (PolygonGroupImportedMaterialSlotNames[PolygonGroupID] == ImportedMaterialSlotName)
						{
							ExistingPolygonGroup = PolygonGroupID;
							break;
						}
					}
					if (ExistingPolygonGroup == -1)
					{
						ExistingPolygonGroup = MD.CreatePolygonGroup();
						PolygonGroupImportedMaterialSlotNames[ExistingPolygonGroup] = ImportedMaterialSlotName;
					}
					PolygonGroupMapping.insert(std::make_pair(RealMaterialIndex, ExistingPolygonGroup));
				}

				std::vector<MeshDescription::ContourPoint> Contours;
				{
					for (uint32 PolygonEdgeNumber = 0; PolygonEdgeNumber < (uint32)PolygonVertexCount; ++PolygonEdgeNumber)
					{
						Contours.push_back(MeshDescription::ContourPoint());
						uint32 ContourPointIndex = Contours.size() - 1;
						MeshDescription::ContourPoint& CP = Contours[ContourPointIndex];
						uint32 CornerIndices[2];
						CornerIndices[0] = (PolygonEdgeNumber + 0) % PolygonVertexCount;
						CornerIndices[1] = (PolygonEdgeNumber + 1) % PolygonVertexCount;

						int EdgeVertexIDs[2];
						EdgeVertexIDs[0] = CornerVerticesIDs[CornerIndices[0]];
						EdgeVertexIDs[1] = CornerVerticesIDs[CornerIndices[1]];

						int MatchEdgeId = MD.GetVertexPairEdge(EdgeVertexIDs[0], EdgeVertexIDs[1]);
						if (MatchEdgeId == -1)
						{
							MatchEdgeId = MD.CreateEdge(EdgeVertexIDs[0], EdgeVertexIDs[1]);
						}
						CP.EdgeID = MatchEdgeId;
						CP.VertexInstanceID = CornerInstanceIDs[CornerIndices[0]];

						int32 EdgeIndex = INDEX_NONE;
						uint64 CompactedKey = (((uint64)EdgeVertexIDs[0] << 32) | ((uint64)EdgeVertexIDs[1]));
						if (RemapEdgeID.find(CompactedKey) != RemapEdgeID.end())
						{
							EdgeIndex = RemapEdgeID[CompactedKey];
						}
						else
						{
							EdgeIndex = lMesh->GetMeshEdgeIndexForPolygon(PolygonIndex, PolygonEdgeNumber);
						}

						EdgeCreaseSharpnesses[MatchEdgeId] = (float)lMesh->GetEdgeCreaseInfo(EdgeIndex);
						if (!EdgeHardnesses[MatchEdgeId])
						{
							if (bSmootingAvaliables && SmoothingInfo)
							{
								if (SmoothingMappingMode == FbxLayerElement::eByEdge)
								{
									int32 lSmoothingIndex = (SmoothingReferenceMode == FbxLayerElement::eDirect) ? EdgeIndex : SmoothingInfo->GetIndexArray().GetAt(EdgeIndex);
									//Set the hard edges
									EdgeHardnesses[MatchEdgeId] = (SmoothingInfo->GetDirectArray().GetAt(lSmoothingIndex) == 0);
								}
								else
								{
									//AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Warning, FText::Format(LOCTEXT("Error_UnsupportedSmoothingGroup", "Unsupported Smoothing group mapping mode on mesh  '{0}'"), FText::FromString(Mesh->GetName()))), FFbxErrors::Generic_Mesh_UnsupportingSmoothingGroup);
								}
							}
							else
							{
								//When there is no smoothing group we set all edge to hard (faceted mesh)
								EdgeHardnesses[MatchEdgeId] = true;
							}
						}
					}
				}

				int PolygonGroupID = PolygonGroupID = PolygonGroupMapping[RealMaterialIndex];
				const int NewPolygonID = MD.CreatePolygon(PolygonGroupID, Contours);
				MeshPolygon& Polygon = MD.GetPolygon(NewPolygonID);
				MD.ComputePolygonTriangulation(NewPolygonID, Polygon.Triangles);
			}
			lMesh->EndGetMeshEdgeIndexForPolygon();

			fbxUVs.CleanUp();
		}
	}

	delete GeometryConverter;
	GeometryConverter = nullptr;

	Mesh->PostLoad();

	return Mesh;
}
void ProcessImportMeshMaterials(std::vector<FSkeletalMaterial>& Materials, FSkeletalMeshImportData& ImportData)
{
	std::vector<VMaterial>&	ImportedMaterials = ImportData.Materials;

	// If direct linkup of materials is requested, try to find them here - to get a texture name from a 
	// material name, cut off anything in front of the dot (beyond are special flags).
	Materials.clear();
	int32 SkinOffset = INDEX_NONE;

	for (uint32 MatIndex = 0; MatIndex < ImportedMaterials.size(); ++MatIndex)
	{
		const VMaterial& ImportedMaterial = ImportedMaterials[MatIndex];

		UMaterial* Material = NULL;
		std::string MaterialNameNoSkin = ImportedMaterial.MaterialImportName;
// 		if (ImportedMaterial.Material.IsValid())
// 		{
// 			Material = ImportedMaterial.Material.Get();
// 		}
// 		else
		{
			const std::string& MaterialName = ImportedMaterial.MaterialImportName;
			MaterialNameNoSkin = MaterialName;
			Material = UMaterial::GetDefaultMaterial(MD_Surface);// FindObject<UMaterialInterface>(ANY_PACKAGE, *MaterialName);
// 			if (Material == nullptr)
// 			{
// 				SkinOffset = MaterialName.Find(TEXT("_skin"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
// 				if (SkinOffset != INDEX_NONE)
// 				{
// 					FString SkinXXNumber = MaterialName.Right(MaterialName.Len() - (SkinOffset + 1)).RightChop(4);
// 					if (SkinXXNumber.IsNumeric())
// 					{
// 						MaterialNameNoSkin = MaterialName.LeftChop(MaterialName.Len() - SkinOffset);
// 						Material = FindObject<UMaterialInterface>(ANY_PACKAGE, *MaterialNameNoSkin);
// 					}
// 				}
// 			}
		}

		const bool bEnableShadowCasting = true;
		Materials.push_back(FSkeletalMaterial(Material, MaterialNameNoSkin));
	}
}

USkeletalMesh* FBXImporter::ImportSkeletalMesh(class AActor* InOwner, const char* pFileName)
{
	ImportOptions = new FBXImportOptions();
	ImportOptions->bImportScene = false;
	ImportOptions->bBakePivotInVertex = false;
	ImportOptions->bTransformVertexToAbsolute = true;
	ImportOptions->bPreserveSmoothingGroups = false;
	ImportOptions->NormalImportMethod = FBXNIM_ComputeNormals;
	ImportOptions->NormalGenerationMethod = EFBXNormalGenerationMethod::MikkTSpace;

	SDKManager = FbxManager::Create();

	FbxIOSettings* lIOSetting = FbxIOSettings::Create(SDKManager, IOSROOT);
	SDKManager->SetIOSettings(lIOSetting);

	FbxImporter* lImporter = FbxImporter::Create(SDKManager, "MeshImporter");
	if (!lImporter->Initialize(pFileName, -1, SDKManager->GetIOSettings()))
	{
		X_LOG("Call to FbxImporter::Initialize() failed.\n");
		X_LOG("Error returned: %s \n\n", lImporter->GetStatus().GetErrorString());
		return NULL;
	}

	fbxScene = FbxScene::Create(SDKManager, "Mesh");
	lImporter->Import(fbxScene);
	lImporter->Destroy();

	FbxAxisSystem::ECoordSystem CoordSystem = FbxAxisSystem::eRightHanded;
	FbxAxisSystem::EUpVector UpVector = FbxAxisSystem::eZAxis;
	FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector) - FbxAxisSystem::eParityOdd;

	FbxAxisSystem UnrealImportAxis(UpVector, FrontVector, CoordSystem);
	FbxAxisSystem SourceSetup = fbxScene->GetGlobalSettings().GetAxisSystem();

	if (SourceSetup != UnrealImportAxis)
	{
		FbxRootNodeUtility::RemoveAllFbxRoots(fbxScene);
		UnrealImportAxis.ConvertScene(fbxScene);
		FbxAMatrix JointOrientationMatrix;
		JointOrientationMatrix.SetIdentity();
// 		if (GetImportOptions()->bForceFrontXAxis)
// 		{
// 			JointOrientationMatrix.SetR(FbxVector4(-90.0, -90.0, 0.0));
// 		}
		SetJointPostConversionMatrix(JointOrientationMatrix);
	}

	std::vector<FbxNode*> lOutMeshArray;
	FillFbxArray(fbxScene->GetRootNode(), lOutMeshArray);

	GeometryConverter = new FbxGeometryConverter(SDKManager);

	// 	for (int Index = 0; Index < (int)lOutMeshArray.size(); ++Index)
	// 	{
	// 		FbxNode* lNode = lOutMeshArray[Index];
	// 		FbxSkeleton* lSkeleton = lNode->GetSkeleton();
	// 	}

	std::vector<FbxMaterial> MeshMaterials;

	FSkeletalMeshImportData TempData;
	// Fill with data from buffer - contains the full .FBX file. 	
	FSkeletalMeshImportData* SkelMeshImportDataPtr = &TempData;

	if (!FillSkeletalMeshImportData(lOutMeshArray, NULL, SkelMeshImportDataPtr))
	{
		X_LOG("Get Import Data has failed.");
		return NULL;
	}

	USkeletalMesh* NewSekeletalMesh = new USkeletalMesh(InOwner);

	// process materials from import data
	ProcessImportMeshMaterials(NewSekeletalMesh->Materials, *SkelMeshImportDataPtr);

	// process reference skeleton from import data
	int32 SkeletalDepth = 0;
	if (!ProcessImportMeshSkeleton(NewSekeletalMesh->Skeleton, NewSekeletalMesh->RefSkeleton, SkeletalDepth, *SkelMeshImportDataPtr))
	{
		//SkeletalMesh->ClearFlags(RF_Standalone);
		//SkeletalMesh->Rename(NULL, GetTransientPackage());
		return nullptr;
	}


	// process bone influences from import data
	//ProcessImportMeshInfluences(*SkelMeshImportDataPtr);


	SkeletalMeshModel *ImportedResource = NewSekeletalMesh->GetImportedModel();
	assert(ImportedResource->LODModels.size() == 0);
	ImportedResource->LODModels.clear();
	ImportedResource->LODModels.push_back(new FSkeletalMeshLODModel());

	NewSekeletalMesh->ResetLODInfo();
	FSkeletalMeshLODInfo& NewLODInfo = NewSekeletalMesh->AddLODInfo();

	FSkeletalMeshLODModel& LODModel = *ImportedResource->LODModels[0];

	LODModel.NumTexCoords = FMath::Max<uint32>(1, SkelMeshImportDataPtr->NumTexCoords);

	if (/*ImportSkeletalMeshArgs.bCreateRenderData*/true)
	{
		std::vector<FVector> LODPoints;
		std::vector<FMeshWedge> LODWedges;
		std::vector<FMeshFace> LODFaces;
		std::vector<FVertInfluence> LODInfluences;
		std::vector<int32> LODPointToRawMap;
		SkelMeshImportDataPtr->CopyLODImportData(LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);

		MeshBuildOptions BuildOptions;
		BuildOptions.OverlappingThresholds = ImportOptions->OverlappingThresholds;
		BuildOptions.bComputeNormals = !ImportOptions->ShouldImportNormals() || !SkelMeshImportDataPtr->bHasNormals;
		BuildOptions.bComputeTangents = !ImportOptions->ShouldImportTangents() || !SkelMeshImportDataPtr->bHasTangents;
		BuildOptions.bUseMikkTSpace = (ImportOptions->NormalGenerationMethod == EFBXNormalGenerationMethod::MikkTSpace) && (!ImportOptions->ShouldImportNormals() || !ImportOptions->ShouldImportTangents());
		BuildOptions.bRemoveDegenerateTriangles = false;

		std::vector<std::string> WarningMessages;
		std::vector<std::string> WarningNames;
		// Create actual rendering data.
		bool bBuildSuccess = BuildSkeletalMesh(*ImportedResource->LODModels[0], NewSekeletalMesh->RefSkeleton, LODInfluences, LODWedges, LODFaces, LODPoints, LODPointToRawMap, BuildOptions, &WarningMessages, &WarningNames);
		
		if (!bBuildSuccess)
		{
			return NULL;
		}
	}

	const uint32 NumSections = LODModel.Sections.size();


	if (/*ImportSkeletalMeshArgs.LodIndex == 0*/true)
	{
		// see if we have skeleton set up
		// if creating skeleton, create skeleeton
		//USkeleton* Skeleton = ImportOptions->SkeletonForAnimation;
		//if (Skeleton == NULL)
		{
			//NewSekeletalMesh->Skeleton = new USkeleton();
			USkeleton* Skeleton = new USkeleton();
			NewSekeletalMesh->Skeleton = Skeleton;
			Skeleton->RecreateBoneTree(NewSekeletalMesh);
		}

	}

	NewSekeletalMesh->PostLoad();

	return NewSekeletalMesh;
}

bool FBXImporter::FillSkeletalMeshImportData(std::vector<FbxNode*>& NodeArray, std::vector<FbxShape*> *FbxShapeArray, FSkeletalMeshImportData* OutData)
{
	if (NodeArray.size() == 0) return false;

	FbxNode* Node = NodeArray[0];
	FbxMesh* FbxMesh = Node->GetMesh();

	FSkeletalMeshImportData* SkelMeshImportDataPtr = OutData;

	std::vector<FbxNode*> SortedLinkArray;
	FbxArray<FbxAMatrix> GlobalsPerLink;
	bool bUseTime0AsRefPose = true;
	if (!ImportBone(NodeArray,*SkelMeshImportDataPtr, SortedLinkArray, bUseTime0AsRefPose,NULL))
	{
		X_LOG("FbxSkeletaLMeshimport_MultipleRootFound");
		return false;
	}

// 	FbxNode* SceneRootNode = Scene->GetRootNode();
// 	if (SceneRootNode && TemplateImportData)
// 	{
// 		ApplyTransformSettingsToFbxNode(SceneRootNode, TemplateImportData);
// 	}

	// Create a list of all unique fbx materials.  This needs to be done as a separate pass before reading geometry
	// so that we know about all possible materials before assigning material indices to each triangle
	std::vector<FbxSurfaceMaterial*> FbxMaterials;
	for (uint32 NodeIndex = 0; NodeIndex < NodeArray.size(); ++NodeIndex)
	{
		Node = NodeArray[NodeIndex];

		int32 MaterialCount = Node->GetMaterialCount();

		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			FbxSurfaceMaterial* FbxMaterial = Node->GetMaterial(MaterialIndex);
			if (std::find(FbxMaterials.begin(), FbxMaterials.end(), FbxMaterial) == FbxMaterials.end())
			{
				FbxMaterials.push_back(FbxMaterial);

				VMaterial NewMaterial;

				NewMaterial.MaterialImportName = FbxMaterial->GetName();
				// Add an entry for each unique material
				SkelMeshImportDataPtr->Materials.push_back(NewMaterial);
			}
		}
	}

	for (uint32 NodeIndex = 0; NodeIndex < NodeArray.size(); ++NodeIndex)
	{
		Node = NodeArray[NodeIndex];
		FbxNode *RootNode = NodeArray[0];
		FbxMesh = Node->GetMesh();
		FbxSkin* Skin = (FbxSkin*)FbxMesh->GetDeformer(0, FbxDeformer::eSkin);
		FbxShape* FbxShape = nullptr;
		if (FbxShapeArray)
		{
			FbxShape = (*FbxShapeArray)[NodeIndex];
		}

		// NOTE: This function may invalidate FbxMesh and set it to point to a an updated version
		if (!FillSkelMeshImporterFromFbx(*SkelMeshImportDataPtr, FbxMesh, Skin, FbxShape, SortedLinkArray, FbxMaterials, RootNode))
		{
			return false;
		}

		if (SkelMeshImportDataPtr->bUseT0AsRefPose && SkelMeshImportDataPtr->bDiffPose && !ImportOptions->bImportScene)
		{
			// deform skin vertex to the frame 0 from bind pose
			SkinControlPointsToPose(*SkelMeshImportDataPtr, FbxMesh, FbxShape, true);
		}
	}

	//CleanUpUnusedMaterials(*SkelMeshImportDataPtr);

// 	if (LastImportedMaterialNames.Num() > 0)
// 	{
// 		SetMaterialOrderByName(*SkelMeshImportDataPtr, LastImportedMaterialNames);
// 	}
// 	else
// 	{
// 		// reorder material according to "SKinXX" in material name
// 		SetMaterialSkinXXOrder(*SkelMeshImportDataPtr);
// 	}

	if (ImportOptions->bPreserveSmoothingGroups)
	{
		bool bDuplicateUnSmoothWedges = (ImportOptions->NormalGenerationMethod != EFBXNormalGenerationMethod::MikkTSpace);
		//DoUnSmoothVerts(*SkelMeshImportDataPtr, bDuplicateUnSmoothWedges);
	}
	else
	{
		SkelMeshImportDataPtr->PointToRawMap.resize(SkelMeshImportDataPtr->Points.size());
		for (uint32 PointIdx = 0; PointIdx < SkelMeshImportDataPtr->Points.size(); PointIdx++)
		{
			SkelMeshImportDataPtr->PointToRawMap[PointIdx] = PointIdx;
		}
	}
	return true;
}

bool FBXImporter::ImportBone(std::vector<FbxNode*>& NodeArray, FSkeletalMeshImportData &ImportData, std::vector<FbxNode*> &SortedLinks, bool& bUseTime0AsRefPose, FbxNode *SkeletalMeshNode)
{
	int32 SkelType = 0; // 0 for skeletal mesh, 1 for rigid mesh
	FbxNode* Link = NULL;
	FbxArray<FbxPose*> PoseArray;
	std::vector<FbxCluster*> ClusterArray;

	if (NodeArray[0]->GetMesh()->GetDeformerCount(FbxDeformer::eSkin) == 0)
	{
		SkelType = 1;
		Link = NodeArray[0];
		RecursiveBuildSkeleton(GetRootSkeleton(Link), SortedLinks);
	}
	else
	{
		// get bindpose and clusters from FBX skeleton

		// let's put the elements to their bind pose! (and we restore them after
		// we have built the ClusterInformation.
		int32 Default_NbPoses = SDKManager->GetBindPoseCount(fbxScene);
		// If there are no BindPoses, the following will generate them.
		//SdkManager->CreateMissingBindPoses(Scene);

		//if we created missing bind poses, update the number of bind poses
		int32 NbPoses = SDKManager->GetBindPoseCount(fbxScene);

		if (NbPoses != Default_NbPoses)
		{
			X_LOG("The imported scene has no initial binding position (Bind Pose) for the skin. The plug-in will compute one automatically. However, it may create unexpected results.");
		}
		//
		// create the bones / skinning
		//
		for (uint32 i = 0; i < NodeArray.size(); i++)
		{
			FbxMesh* FbxMesh = NodeArray[i]->GetMesh();
			const int32 SkinDeformerCount = FbxMesh->GetDeformerCount(FbxDeformer::eSkin);
			for (int32 DeformerIndex = 0; DeformerIndex < SkinDeformerCount; DeformerIndex++)
			{
				FbxSkin* Skin = (FbxSkin*)FbxMesh->GetDeformer(DeformerIndex, FbxDeformer::eSkin);
				for (int32 ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ClusterIndex++)
				{
					ClusterArray.push_back(Skin->GetCluster(ClusterIndex));
				}
			}
		}

		if (ClusterArray.size() == 0)
		{
			X_LOG("No associated clusters");
			return false;
		}

		// get bind pose
		if (RetrievePoseFromBindPose(NodeArray, PoseArray) == false)
		{
			X_LOG("Getting valid bind pose failed. Try to recreate bind pose");
			// if failed, delete bind pose, and retry.
			const int32 PoseCount = fbxScene->GetPoseCount();
			for (int32 PoseIndex = PoseCount - 1; PoseIndex >= 0; --PoseIndex)
			{
				FbxPose* CurrentPose = fbxScene->GetPose(PoseIndex);

				// current pose is bind pose, 
				if (CurrentPose && CurrentPose->IsBindPose())
				{
					fbxScene->RemovePose(PoseIndex);
					CurrentPose->Destroy();
				}
			}

			SDKManager->CreateMissingBindPoses(fbxScene);
			if (RetrievePoseFromBindPose(NodeArray, PoseArray) == false)
			{
				X_LOG("Recreating bind pose failed.");
			}
			else
			{
				X_LOG("Recreating bind pose succeeded.");
			}
		}

		// recurse through skeleton and build ordered table
		BuildSkeletonSystem(ClusterArray, SortedLinks);
	}

	if (SortedLinks.size() == 0)
	{
		X_LOG("FbxSkeletaLMeshimport_NoBone, '{0}' has no bones");
		return false;
	}
	if (!bUseTime0AsRefPose && PoseArray.GetCount() == 0)
	{
		bUseTime0AsRefPose = true;
	}

	uint32 LinkIndex;
	// Check for duplicate bone names and issue a warning if found
	for (LinkIndex = 0; LinkIndex < SortedLinks.size(); ++LinkIndex)
	{
		Link = SortedLinks[LinkIndex];

		for (uint32 AltLinkIndex = LinkIndex + 1; AltLinkIndex < SortedLinks.size(); ++AltLinkIndex)
		{
			FbxNode* AltLink = SortedLinks[AltLinkIndex];

			if (strcmp(Link->GetName(), AltLink->GetName()) == 0)
			{
				X_LOG("Error, Could not import %s.\nDuplicate bone name found (%s). Each bone must have a unique name.",NodeArray[0]->GetName(), Link->GetName());
				return false;
			}
		}
	}

	FbxArray<FbxAMatrix> GlobalsPerLink;
	GlobalsPerLink.Grow(SortedLinks.size());
	GlobalsPerLink[0].SetIdentity();

	bool GlobalLinkFoundFlag;
	FbxVector4 LocalLinkT;
	FbxQuaternion LocalLinkQ;
	FbxVector4	LocalLinkS;

	bool bAnyLinksNotInBindPose = false;
	std::string LinksWithoutBindPoses;
	int32 NumberOfRoot = 0;

	int32 RootIdx = -1;

	for (LinkIndex = 0; LinkIndex < SortedLinks.size(); LinkIndex++)
	{
		ImportData.RefBonesBinary.push_back(VBone());

		Link = SortedLinks[LinkIndex];

		int32 ParentIndex = -1; // base value for root if no parent found
		FbxNode* LinkParent = Link->GetParent();
		if (LinkIndex)
		{
			for (uint32 ll = 0; ll < LinkIndex; ++ll) // <LinkIndex because parent is guaranteed to be before child in sortedLink
			{
				FbxNode* Otherlink = SortedLinks[ll];
				if (Otherlink == LinkParent)
				{
					ParentIndex = ll;
					break;
				}
			}
		}

		if (ParentIndex == -1)
		{
			++NumberOfRoot;
			RootIdx = LinkIndex;
			if (NumberOfRoot > 1)
			{
				X_LOG("Multiple roots are found in the bone hierarchy. We only support single root bone.");
				return false;
			}
		}

		GlobalLinkFoundFlag = false;
		if (!SkelType) //skeletal mesh
		{
			// there are some links, they have no cluster, but in bindpose
			if (PoseArray.GetCount())
			{
				for (int32 PoseIndex = 0; PoseIndex < PoseArray.GetCount(); PoseIndex++)
				{
					int32 PoseLinkIndex = PoseArray[PoseIndex]->Find(Link);
					if (PoseLinkIndex >= 0)
					{
						FbxMatrix NoneAffineMatrix = PoseArray[PoseIndex]->GetMatrix(PoseLinkIndex);
						FbxAMatrix Matrix = *(FbxAMatrix*)(double*)&NoneAffineMatrix;
						GlobalsPerLink[LinkIndex] = Matrix;
						GlobalLinkFoundFlag = true;
						break;
					}
				}
			}

			if (!GlobalLinkFoundFlag)
			{
				// since now we set use time 0 as ref pose this won't unlikely happen
				// but leaving it just in case it still has case where it's missing partial bind pose
				if (!bUseTime0AsRefPose /*&& !bDisableMissingBindPoseWarning*/)
				{
					bAnyLinksNotInBindPose = true;
					LinksWithoutBindPoses += Link->GetName();
					LinksWithoutBindPoses += "  \n";
				}

				for (uint32 ClusterIndex = 0; ClusterIndex < ClusterArray.size(); ClusterIndex++)
				{
					FbxCluster* Cluster = ClusterArray[ClusterIndex];
					if (Link == Cluster->GetLink())
					{
						Cluster->GetTransformLinkMatrix(GlobalsPerLink[LinkIndex]);
						GlobalLinkFoundFlag = true;
						break;
					}
				}
			}

			if (!GlobalLinkFoundFlag)
			{
				GlobalsPerLink[LinkIndex] = Link->EvaluateGlobalTransform();
			}

			if (bUseTime0AsRefPose /*&& !ImportOptions->bImportScene*/)
			{
				FbxAMatrix& T0Matrix = fbxScene->GetAnimationEvaluator()->GetNodeGlobalTransform(Link, 0);
				if (GlobalsPerLink[LinkIndex] != T0Matrix)
				{
					//bOutDiffPose = true;
				}

				GlobalsPerLink[LinkIndex] = T0Matrix;
			}

			//Add the join orientation
			GlobalsPerLink[LinkIndex] = GlobalsPerLink[LinkIndex] * GetJointPostConversionMatrix();
			if (LinkIndex)
			{
				FbxAMatrix	Matrix;
				Matrix = GlobalsPerLink[ParentIndex].Inverse() * GlobalsPerLink[LinkIndex];
				LocalLinkT = Matrix.GetT();
				LocalLinkQ = Matrix.GetQ();
				LocalLinkS = Matrix.GetS();
			}
			else	// skeleton root
			{
				// for root, this is global coordinate
				LocalLinkT = GlobalsPerLink[LinkIndex].GetT();
				LocalLinkQ = GlobalsPerLink[LinkIndex].GetQ();
				LocalLinkS = GlobalsPerLink[LinkIndex].GetS();
			}

			// set bone
			VBone& Bone = ImportData.RefBonesBinary[LinkIndex];
			std::string BoneName;

			const char* LinkName = Link->GetName();
			BoneName = LinkName;
			Bone.Name = BoneName;

			VJointPos& JointMatrix = Bone.BonePos;
			FbxSkeleton* Skeleton = Link->GetSkeleton();
			if (Skeleton)
			{
				JointMatrix.Length = ConvertDist(Skeleton->LimbLength.Get());
				JointMatrix.XSize = ConvertDist(Skeleton->Size.Get());
				JointMatrix.YSize = ConvertDist(Skeleton->Size.Get());
				JointMatrix.ZSize = ConvertDist(Skeleton->Size.Get());
			}
			else
			{
				JointMatrix.Length = 1.;
				JointMatrix.XSize = 100.;
				JointMatrix.YSize = 100.;
				JointMatrix.ZSize = 100.;
			}

			// get the link parent and children
			Bone.ParentIndex = ParentIndex;
			Bone.NumChildren = 0;
			for (int32 ChildIndex = 0; ChildIndex < Link->GetChildCount(); ChildIndex++)
			{
				FbxNode* Child = Link->GetChild(ChildIndex);
				if (IsUnrealBone(Child))
				{
					Bone.NumChildren++;
				}
			}

			JointMatrix.Transform.SetTranslation(ConvertPos(LocalLinkT));
			JointMatrix.Transform.SetRotation(ConvertRotToQuat(LocalLinkQ));
			JointMatrix.Transform.SetScale3D(ConvertScale(LocalLinkS));
		}
	}

	return true;
}


bool FBXImporter::FillSkelMeshImporterFromFbx(FSkeletalMeshImportData& ImportData, FbxMesh*& Mesh, FbxSkin* Skin, FbxShape* FbxShape , std::vector<FbxNode*> &SortedLinks, const std::vector<FbxSurfaceMaterial*>& FbxMaterials, FbxNode *RootNode)
{
	FbxNode* Node = Mesh->GetNode();

	//remove the bad polygons before getting any data from mesh
	Mesh->RemoveBadPolygons();

	FbxLayer* BaseLayer = Mesh->GetLayer(0);
	if (BaseLayer == NULL)
	{
		X_LOG("There is no geometry information in mesh");
		return false;
	}

	// Do some checks before proceeding, check to make sure the number of bones does not exceed the maximum supported
	if (SortedLinks.size() > 65536/*MAX_BONES*/)
	{
		X_LOG("'%s' mesh has '%d' bones which exceeds the maximum allowed bone count of %d.", Node->GetName(), (int)SortedLinks.size(), 65536);
		return false;
	}

	//
	//	store the UVs in arrays for fast access in the later looping of triangles 
	//
	// mapping from UVSets to Fbx LayerElementUV
	// Fbx UVSets may be duplicated, remove the duplicated UVSets in the mapping 
	int32 LayerCount = Mesh->GetLayerCount();
	std::vector<std::string> UVSets;

	UVSets.clear();
	if (LayerCount > 0)
	{
		int32 UVLayerIndex;
		for (UVLayerIndex = 0; UVLayerIndex < LayerCount; UVLayerIndex++)
		{
			FbxLayer* lLayer = Mesh->GetLayer(UVLayerIndex);
			int32 UVSetCount = lLayer->GetUVSetCount();
			if (UVSetCount)
			{
				FbxArray<FbxLayerElementUV const*> EleUVs = lLayer->GetUVSets();
				for (int32 UVIndex = 0; UVIndex < UVSetCount; UVIndex++)
				{
					FbxLayerElementUV const* ElementUV = EleUVs[UVIndex];
					if (ElementUV)
					{
						const char* UVSetName = ElementUV->GetName();
						std::string LocalUVSetName = UVSetName;
						if (LocalUVSetName.size() == 0)
						{
							LocalUVSetName = std::string("UVmap_") + std::to_string(UVLayerIndex);
						}
						UVSets.push_back(LocalUVSetName);
					}
				}
			}
		}
	}

	// If the the UV sets are named using the following format (UVChannel_X; where X ranges from 1 to 4)
	// we will re-order them based on these names.  Any UV sets that do not follow this naming convention
	// will be slotted into available spaces.
	if (UVSets.size() > 0)
	{
		for (uint32 ChannelNumIdx = 0; ChannelNumIdx < 4; ChannelNumIdx++)
		{
			std::string ChannelName = std::string("UVChannel_") + std::to_string(ChannelNumIdx + 1);
			auto it = std::find(UVSets.begin(), UVSets.end(), ChannelName);
			int32 SetIdx = it == UVSets.end() ? -1 : it - UVSets.begin();

			// If the specially formatted UVSet name appears in the list and it is in the wrong spot,
			// we will swap it into the correct spot.
			if (SetIdx != -1 && SetIdx != ChannelNumIdx)
			{
				// If we are going to swap to a position that is outside the bounds of the
				// array, then we pad out to that spot with empty data.
				for (uint32 ArrSize = UVSets.size(); ArrSize < ChannelNumIdx + 1; ArrSize++)
				{
					UVSets.push_back(std::string(""));
				}
				//Swap the entry into the appropriate spot.
				std::swap(UVSets[SetIdx], UVSets[ChannelNumIdx]);
			}
		}
	}

	//std::vector<UMaterialInterface*> Materials;
// 	if (ImportOptions->bImportMaterials)
// 	{
// 		bool bForSkeletalMesh = true;
// 		CreateNodeMaterials(Node, Materials, UVSets, bForSkeletalMesh);
// 	}
// 	else if (ImportOptions->bImportTextures)
// 	{
// 		ImportTexturesFromNode(Node);
// 	}

	std::vector<int32> MaterialMapping;
	int32 MaterialCount = Node->GetMaterialCount();

	MaterialMapping.resize(MaterialCount);

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		FbxSurfaceMaterial* FbxMaterial = Node->GetMaterial(MaterialIndex);

		int32 ExistingMatIndex = -1;
		auto it = std::find(FbxMaterials.begin(), FbxMaterials.end(), FbxMaterial);
		if (it != FbxMaterials.end())
			ExistingMatIndex = it - FbxMaterials.begin();
		//FbxMaterials.Find(FbxMaterial, ExistingMatIndex);
		if (ExistingMatIndex != -1)
		{
			// Reuse existing material
			MaterialMapping[MaterialIndex] = ExistingMatIndex;

			//if (Materials.size() > (MaterialIndex))
			{
				//ImportData.Materials[ExistingMatIndex].Material = Materials[MaterialIndex];
			}
		}
		else
		{
			MaterialMapping[MaterialIndex] = 0;
		}
	}


	if (LayerCount > 0 && ImportOptions->bPreserveSmoothingGroups)
	{
		// Check and see if the smooothing data is valid.  If not generate it from the normals
		BaseLayer = Mesh->GetLayer(0);
		if (BaseLayer)
		{
			const FbxLayerElementSmoothing* SmoothingLayer = BaseLayer->GetSmoothing();

			if (SmoothingLayer)
			{
				bool bValidSmoothingData = false;
				FbxLayerElementArrayTemplate<int32>& Array = SmoothingLayer->GetDirectArray();
				for (int32 SmoothingIndex = 0; SmoothingIndex < Array.GetCount(); ++SmoothingIndex)
				{
					if (Array[SmoothingIndex] != 0)
					{
						bValidSmoothingData = true;
						break;
					}
				}

				if (!bValidSmoothingData && Mesh->GetPolygonVertexCount() > 0)
				{
					GeometryConverter->ComputeEdgeSmoothingFromNormals(Mesh);
				}
			}
		}
	}

	// Must do this before triangulating the mesh due to an FBX bug in TriangulateMeshAdvance
	int32 LayerSmoothingCount = Mesh->GetLayerCount(FbxLayerElement::eSmoothing);
	for (int32 i = 0; i < LayerSmoothingCount; i++)
	{
		GeometryConverter->ComputePolygonSmoothingFromEdgeSmoothing(Mesh, i);
	}

	//
	// Convert data format to unreal-compatible
	//

	if (!Mesh->IsTriangleMesh())
	{
		X_LOG("Triangulating skeletal mesh %s", Node->GetName());

		const bool bReplace = true;
		FbxNodeAttribute* ConvertedNode = GeometryConverter->Triangulate(Mesh, bReplace);
		if (ConvertedNode != NULL && ConvertedNode->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			Mesh = ConvertedNode->GetNode()->GetMesh();
		}
		else
		{
			X_LOG("Unable to triangulate mesh '%s'. Check detail for Ouput Log.", Node->GetName());
			return false;
		}
	}

	// renew the base layer
	BaseLayer = Mesh->GetLayer(0);
	Skin = (FbxSkin*)static_cast<FbxGeometry*>(Mesh)->GetDeformer(0, FbxDeformer::eSkin);

	//
	//	store the UVs in arrays for fast access in the later looping of triangles 
	//
	uint32 UniqueUVCount = UVSets.size();
	FbxLayerElementUV** LayerElementUV = NULL;
	FbxLayerElement::EReferenceMode* UVReferenceMode = NULL;
	FbxLayerElement::EMappingMode* UVMappingMode = NULL;
	if (UniqueUVCount > 0)
	{
		LayerElementUV = new FbxLayerElementUV*[UniqueUVCount];
		UVReferenceMode = new FbxLayerElement::EReferenceMode[UniqueUVCount];
		UVMappingMode = new FbxLayerElement::EMappingMode[UniqueUVCount];
	}
	else
	{
		X_LOG("Mesh '%s' has no UV set. Creating a default set.", Node->GetName());
	}

	LayerCount = Mesh->GetLayerCount();
	for (uint32 UVIndex = 0; UVIndex < UniqueUVCount; UVIndex++)
	{
		LayerElementUV[UVIndex] = NULL;
		for (int32 UVLayerIndex = 0; UVLayerIndex < LayerCount; UVLayerIndex++)
		{
			FbxLayer* lLayer = Mesh->GetLayer(UVLayerIndex);
			int32 UVSetCount = lLayer->GetUVSetCount();
			if (UVSetCount)
			{
				FbxArray<FbxLayerElementUV const*> EleUVs = lLayer->GetUVSets();
				for (int32 FbxUVIndex = 0; FbxUVIndex < UVSetCount; FbxUVIndex++)
				{
					FbxLayerElementUV const* ElementUV = EleUVs[FbxUVIndex];
					if (ElementUV)
					{
						const char* UVSetName = ElementUV->GetName();
						std::string LocalUVSetName = UVSetName;
						if (LocalUVSetName.size() == 0)
						{
							LocalUVSetName = std::string("UVmap_") + std::to_string(UVLayerIndex);
						}
						if (LocalUVSetName == UVSets[UVIndex])
						{
							LayerElementUV[UVIndex] = const_cast<FbxLayerElementUV*>(ElementUV);
							UVReferenceMode[UVIndex] = LayerElementUV[UVIndex]->GetReferenceMode();
							UVMappingMode[UVIndex] = LayerElementUV[UVIndex]->GetMappingMode();
							break;
						}
					}
				}
			}
		}
	}

	//
	// get the smoothing group layer
	//
	bool bSmoothingAvailable = false;

	FbxLayerElementSmoothing const* SmoothingInfo = BaseLayer->GetSmoothing();
	FbxLayerElement::EReferenceMode SmoothingReferenceMode(FbxLayerElement::eDirect);
	FbxLayerElement::EMappingMode SmoothingMappingMode(FbxLayerElement::eByEdge);
	if (SmoothingInfo)
	{
		if (SmoothingInfo->GetMappingMode() == FbxLayerElement::eByEdge)
		{
			if (!GeometryConverter->ComputePolygonSmoothingFromEdgeSmoothing(Mesh))
			{
				X_LOG("Unable to fully convert the smoothing groups for mesh '%d'", Mesh->GetName());
				bSmoothingAvailable = false;
			}
		}

		if (SmoothingInfo->GetMappingMode() == FbxLayerElement::eByPolygon)
		{
			bSmoothingAvailable = true;
		}


		SmoothingReferenceMode = SmoothingInfo->GetReferenceMode();
		SmoothingMappingMode = SmoothingInfo->GetMappingMode();
	}

	//
	//	get the "material index" layer
	//
	FbxLayerElementMaterial* LayerElementMaterial = BaseLayer->GetMaterials();
	FbxLayerElement::EMappingMode MaterialMappingMode = LayerElementMaterial ?
		LayerElementMaterial->GetMappingMode() : FbxLayerElement::eByPolygon;

	UniqueUVCount = FMath::Min<uint32>(UniqueUVCount, 4);

	// One UV set is required but only import up to MAX_TEXCOORDS number of uv layers
	ImportData.NumTexCoords = FMath::Max<uint32>(ImportData.NumTexCoords, UniqueUVCount);

	//
	// get the first vertex color layer
	//
	FbxLayerElementVertexColor* LayerElementVertexColor = BaseLayer->GetVertexColors();
	FbxLayerElement::EReferenceMode VertexColorReferenceMode(FbxLayerElement::eDirect);
	FbxLayerElement::EMappingMode VertexColorMappingMode(FbxLayerElement::eByControlPoint);
	if (LayerElementVertexColor)
	{
		VertexColorReferenceMode = LayerElementVertexColor->GetReferenceMode();
		VertexColorMappingMode = LayerElementVertexColor->GetMappingMode();
		ImportData.bHasVertexColors = true;
	}

	//
	// get the first normal layer
	//
	FbxLayerElementNormal* LayerElementNormal = BaseLayer->GetNormals();
	FbxLayerElementTangent* LayerElementTangent = BaseLayer->GetTangents();
	FbxLayerElementBinormal* LayerElementBinormal = BaseLayer->GetBinormals();

	//whether there is normal, tangent and binormal data in this mesh
	bool bHasNormalInformation = LayerElementNormal != NULL;
	bool bHasTangentInformation = LayerElementTangent != NULL && LayerElementBinormal != NULL;

	ImportData.bHasNormals = bHasNormalInformation;
	ImportData.bHasTangents = bHasTangentInformation;

	FbxLayerElement::EReferenceMode NormalReferenceMode(FbxLayerElement::eDirect);
	FbxLayerElement::EMappingMode NormalMappingMode(FbxLayerElement::eByControlPoint);
	if (LayerElementNormal)
	{
		NormalReferenceMode = LayerElementNormal->GetReferenceMode();
		NormalMappingMode = LayerElementNormal->GetMappingMode();
	}

	FbxLayerElement::EReferenceMode TangentReferenceMode(FbxLayerElement::eDirect);
	FbxLayerElement::EMappingMode TangentMappingMode(FbxLayerElement::eByControlPoint);
	if (LayerElementTangent)
	{
		TangentReferenceMode = LayerElementTangent->GetReferenceMode();
		TangentMappingMode = LayerElementTangent->GetMappingMode();
	}

	//
	// create the points / wedges / faces
	//
	int32 ControlPointsCount = Mesh->GetControlPointsCount();
	uint32 ExistPointNum = ImportData.Points.size();
	FillSkeletalMeshImportPoints(&ImportData, RootNode, Node, FbxShape);

	// Construct the matrices for the conversion from right handed to left handed system
	FbxAMatrix TotalMatrix;
	FbxAMatrix TotalMatrixForNormal;
	TotalMatrix = ComputeSkeletalMeshTotalMatrix(Node, RootNode);
	TotalMatrixForNormal = TotalMatrix.Inverse();
	TotalMatrixForNormal = TotalMatrixForNormal.Transpose();

	bool OddNegativeScale = IsOddNegativeScale(TotalMatrix);

	int32 TriangleCount = Mesh->GetPolygonCount();
	uint32 ExistFaceNum = ImportData.Faces.size();
	ImportData.Faces.resize(TriangleCount);
	uint32 ExistWedgesNum = ImportData.Wedges.size();
	VVertex TmpWedges[3];

	for (uint32 TriangleIndex = ExistFaceNum, LocalIndex = 0; TriangleIndex < ExistFaceNum + TriangleCount; TriangleIndex++, LocalIndex++)
	{

		VTriangle& Triangle = ImportData.Faces[TriangleIndex];

		//
		// smoothing mask
		//
		// set the face smoothing by default. It could be any number, but not zero
		Triangle.SmoothingGroups = 255;
		if (bSmoothingAvailable)
		{
			if (SmoothingInfo)
			{
				if (SmoothingMappingMode == FbxLayerElement::eByPolygon)
				{
					int32 lSmoothingIndex = (SmoothingReferenceMode == FbxLayerElement::eDirect) ? LocalIndex : SmoothingInfo->GetIndexArray().GetAt(LocalIndex);
					Triangle.SmoothingGroups = SmoothingInfo->GetDirectArray().GetAt(lSmoothingIndex);
				}
				else
				{
					X_LOG("Unsupported Smoothing group mapping mode on mesh '%s'", Mesh->GetName());
				}
			}
		}

		for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
		{
			// If there are odd number negative scale, invert the vertex order for triangles
			int32 UnrealVertexIndex = OddNegativeScale ? 2 - VertexIndex : VertexIndex;

			int32 ControlPointIndex = Mesh->GetPolygonVertex(LocalIndex, VertexIndex);
			//
			// normals, tangents and binormals
			//
			if (ImportOptions->ShouldImportNormals() && bHasNormalInformation)
			{
				int32 TmpIndex = LocalIndex * 3 + VertexIndex;
				//normals may have different reference and mapping mode than tangents and binormals
				int32 NormalMapIndex = (NormalMappingMode == FbxLayerElement::eByControlPoint) ?
					ControlPointIndex : TmpIndex;
				int32 NormalValueIndex = (NormalReferenceMode == FbxLayerElement::eDirect) ?
					NormalMapIndex : LayerElementNormal->GetIndexArray().GetAt(NormalMapIndex);

				//tangents and binormals share the same reference, mapping mode and index array
				int32 TangentMapIndex = TmpIndex;

				FbxVector4 TempValue;

				if (ImportOptions->ShouldImportTangents() && bHasTangentInformation)
				{
					TempValue = LayerElementTangent->GetDirectArray().GetAt(TangentMapIndex);
					TempValue = TotalMatrixForNormal.MultT(TempValue);
					Triangle.TangentX[UnrealVertexIndex] = ConvertDir(TempValue);
					Triangle.TangentX[UnrealVertexIndex].Normalize();

					TempValue = LayerElementBinormal->GetDirectArray().GetAt(TangentMapIndex);
					TempValue = TotalMatrixForNormal.MultT(TempValue);
					Triangle.TangentY[UnrealVertexIndex] = -ConvertDir(TempValue);
					Triangle.TangentY[UnrealVertexIndex].Normalize();
				}

				TempValue = LayerElementNormal->GetDirectArray().GetAt(NormalValueIndex);
				TempValue = TotalMatrixForNormal.MultT(TempValue);
				Triangle.TangentZ[UnrealVertexIndex] = ConvertDir(TempValue);
				Triangle.TangentZ[UnrealVertexIndex].Normalize();

			}
			else
			{
				int32 NormalIndex;
				for (NormalIndex = 0; NormalIndex < 3; ++NormalIndex)
				{
					Triangle.TangentX[NormalIndex] = FVector::ZeroVector;
					Triangle.TangentY[NormalIndex] = FVector::ZeroVector;
					Triangle.TangentZ[NormalIndex] = FVector::ZeroVector;
				}
			}
		}

		//
		// material index
		//
		Triangle.MatIndex = 0; // default value
		if (MaterialCount > 0)
		{
			if (LayerElementMaterial)
			{
				switch (MaterialMappingMode)
				{
					// material index is stored in the IndexArray, not the DirectArray (which is irrelevant with 2009.1)
				case FbxLayerElement::eAllSame:
				{
					Triangle.MatIndex = MaterialMapping[LayerElementMaterial->GetIndexArray().GetAt(0)];
				}
				break;
				case FbxLayerElement::eByPolygon:
				{
					int32 Index = LayerElementMaterial->GetIndexArray().GetAt(LocalIndex);
					if (!((int32)MaterialMapping.size() > Index))
					{
						X_LOG("Face material index inconsistency - forcing to 0");
					}
					else
					{
						Triangle.MatIndex = MaterialMapping[Index];
					}
				}
				break;
				}
			}

			// When import morph, we don't check the material index 
			// because we don't import material for morph, so the ImportData.Materials contains zero material
			if (!FbxShape && (Triangle.MatIndex < 0 || Triangle.MatIndex >= FbxMaterials.size()))
			{
				X_LOG("Face material index inconsistency - forcing to 0");
				Triangle.MatIndex = 0;
			}
		}
		ImportData.MaxMaterialIndex = FMath::Max<uint32>(ImportData.MaxMaterialIndex, Triangle.MatIndex);

		Triangle.AuxMatIndex = 0;
		for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
		{
			// If there are odd number negative scale, invert the vertex order for triangles
			int32 UnrealVertexIndex = OddNegativeScale ? 2 - VertexIndex : VertexIndex;

			TmpWedges[UnrealVertexIndex].MatIndex = Triangle.MatIndex;
			TmpWedges[UnrealVertexIndex].VertexIndex = ExistPointNum + Mesh->GetPolygonVertex(LocalIndex, VertexIndex);
			// Initialize all colors to white.
			TmpWedges[UnrealVertexIndex].Color = FColor::White;
		}

		//
		// uvs
		//
		uint32 UVLayerIndex;
		// Some FBX meshes can have no UV sets, so also check the UniqueUVCount
		for (UVLayerIndex = 0; UVLayerIndex < UniqueUVCount; UVLayerIndex++)
		{
			// ensure the layer has data
			if (LayerElementUV[UVLayerIndex] != NULL)
			{
				// Get each UV from the layer
				for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
				{
					// If there are odd number negative scale, invert the vertex order for triangles
					int32 UnrealVertexIndex = OddNegativeScale ? 2 - VertexIndex : VertexIndex;

					int32 lControlPointIndex = Mesh->GetPolygonVertex(LocalIndex, VertexIndex);
					int32 UVMapIndex = (UVMappingMode[UVLayerIndex] == FbxLayerElement::eByControlPoint) ?
						lControlPointIndex : LocalIndex * 3 + VertexIndex;
					int32 UVIndex = (UVReferenceMode[UVLayerIndex] == FbxLayerElement::eDirect) ?
						UVMapIndex : LayerElementUV[UVLayerIndex]->GetIndexArray().GetAt(UVMapIndex);
					FbxVector2	UVVector = LayerElementUV[UVLayerIndex]->GetDirectArray().GetAt(UVIndex);

					TmpWedges[UnrealVertexIndex].UVs[UVLayerIndex].X = static_cast<float>(UVVector[0]);
					TmpWedges[UnrealVertexIndex].UVs[UVLayerIndex].Y = 1.f - static_cast<float>(UVVector[1]);
				}
			}
			else if (UVLayerIndex == 0)
			{
				// Set all UV's to zero.  If we are here the mesh had no UV sets so we only need to do this for the
				// first UV set which always exists.

				for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
				{
					TmpWedges[VertexIndex].UVs[UVLayerIndex].X = 0.0f;
					TmpWedges[VertexIndex].UVs[UVLayerIndex].Y = 0.0f;
				}
			}
		}

		// Read vertex colors if they exist.
		if (LayerElementVertexColor)
		{
			switch (VertexColorMappingMode)
			{
			case FbxLayerElement::eByControlPoint:
			{
				for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
				{
					int32 UnrealVertexIndex = OddNegativeScale ? 2 - VertexIndex : VertexIndex;

					FbxColor VertexColor = (VertexColorReferenceMode == FbxLayerElement::eDirect)
						? LayerElementVertexColor->GetDirectArray().GetAt(Mesh->GetPolygonVertex(LocalIndex, VertexIndex))
						: LayerElementVertexColor->GetDirectArray().GetAt(LayerElementVertexColor->GetIndexArray().GetAt(Mesh->GetPolygonVertex(LocalIndex, VertexIndex)));

					TmpWedges[UnrealVertexIndex].Color = FColor(uint8(255.f*VertexColor.mRed),
						uint8(255.f*VertexColor.mGreen),
						uint8(255.f*VertexColor.mBlue),
						uint8(255.f*VertexColor.mAlpha));
				}
			}
			break;
			case FbxLayerElement::eByPolygonVertex:
			{
				for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
				{
					int32 UnrealVertexIndex = OddNegativeScale ? 2 - VertexIndex : VertexIndex;

					FbxColor VertexColor = (VertexColorReferenceMode == FbxLayerElement::eDirect)
						? LayerElementVertexColor->GetDirectArray().GetAt(LocalIndex * 3 + VertexIndex)
						: LayerElementVertexColor->GetDirectArray().GetAt(LayerElementVertexColor->GetIndexArray().GetAt(LocalIndex * 3 + VertexIndex));

					TmpWedges[UnrealVertexIndex].Color = FColor(uint8(255.f*VertexColor.mRed),
						uint8(255.f*VertexColor.mGreen),
						uint8(255.f*VertexColor.mBlue),
						uint8(255.f*VertexColor.mAlpha));
				}
			}
			break;
			}
		}

		//
		// basic wedges matching : 3 unique per face. TODO Can we do better ?
		//
		for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
		{
			int32 w;

			ImportData.Wedges.push_back(VVertex());
			w = (int32)ImportData.Wedges.size() - 1;
			ImportData.Wedges[w].VertexIndex = TmpWedges[VertexIndex].VertexIndex;
			ImportData.Wedges[w].MatIndex = TmpWedges[VertexIndex].MatIndex;
			ImportData.Wedges[w].Color = TmpWedges[VertexIndex].Color;
			ImportData.Wedges[w].Reserved = 0;
			memcpy(ImportData.Wedges[w].UVs, TmpWedges[VertexIndex].UVs, sizeof(Vector2)*4);

			Triangle.WedgeIndex[VertexIndex] = w;
		}

	}

	// now we can work on a per-cluster basis with good ordering
	if (Skin) // skeletal mesh
	{
		// create influences for each cluster
		int32 ClusterIndex;
		for (ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ClusterIndex++)
		{
			FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
			// When Maya plug-in exports rigid binding, it will generate "CompensationCluster" for each ancestor links.
			// FBX writes these "CompensationCluster" out. The CompensationCluster also has weight 1 for vertices.
			// Unreal importer should skip these clusters.
			if (!Cluster || (strcmp(Cluster->GetUserDataID(), "Maya_ClusterHint") == 0 && strcmp(Cluster->GetUserData(), "CompensationCluster") == 0))
			{
				continue;
			}

			FbxNode* Link = Cluster->GetLink();
			// find the bone index
			int32 BoneIndex = -1;
			for (uint32 LinkIndex = 0; LinkIndex < SortedLinks.size(); LinkIndex++)
			{
				if (Link == SortedLinks[LinkIndex])
				{
					BoneIndex = LinkIndex;
					break;
				}
			}

			//	get the vertex indices
			int32 ControlPointIndicesCount = Cluster->GetControlPointIndicesCount();
			int32* ControlPointIndices = Cluster->GetControlPointIndices();
			double* Weights = Cluster->GetControlPointWeights();

			//	for each vertex index in the cluster
			for (int32 ControlPointIndex = 0; ControlPointIndex < ControlPointIndicesCount; ++ControlPointIndex)
			{
				ImportData.Influences.push_back(VRawBoneInfluence());
				ImportData.Influences.back().BoneIndex = BoneIndex;
				ImportData.Influences.back().Weight = static_cast<float>(Weights[ControlPointIndex]);
				ImportData.Influences.back().VertexIndex = ExistPointNum + ControlPointIndices[ControlPointIndex];
			}
		}
	}
	else // for rigid mesh
	{
		// find the bone index
		int32 BoneIndex = -1;
		for (uint32 LinkIndex = 0; LinkIndex < SortedLinks.size(); LinkIndex++)
		{
			// the bone is the node itself
			if (Node == SortedLinks[LinkIndex])
			{
				BoneIndex = LinkIndex;
				break;
			}
		}

		//	for each vertex in the mesh
		for (int32 ControlPointIndex = 0; ControlPointIndex < ControlPointsCount; ++ControlPointIndex)
		{
			ImportData.Influences.push_back(VRawBoneInfluence());
			ImportData.Influences.back().BoneIndex = BoneIndex;
			ImportData.Influences.back().Weight = 1.0;
			ImportData.Influences.back().VertexIndex = ExistPointNum + ControlPointIndex;
		}
	}

	//
	// clean up
	//
	if (UniqueUVCount > 0)
	{
		delete[] LayerElementUV;
		delete[] UVReferenceMode;
		delete[] UVMappingMode;
	}

	return true; //-V773
}

bool FBXImporter::FillSkeletalMeshImportPoints(FSkeletalMeshImportData* OutData, FbxNode* RootNode, FbxNode* Node, FbxShape* FbxShape)
{
	FbxMesh* FbxMesh = Node->GetMesh();

	const int32 ControlPointsCount = FbxMesh->GetControlPointsCount();
	const int32 ExistPointNum = OutData->Points.size();
	OutData->Points.resize(ControlPointsCount);

	// Construct the matrices for the conversion from right handed to left handed system
	FbxAMatrix TotalMatrix = ComputeSkeletalMeshTotalMatrix(Node, RootNode);

	int32 ControlPointsIndex;
	bool bInvalidPositionFound = false;
	for (ControlPointsIndex = 0; ControlPointsIndex < ControlPointsCount; ControlPointsIndex++)
	{
		FbxVector4 Position;
		if (FbxShape)
		{
			Position = FbxShape->GetControlPoints()[ControlPointsIndex];
		}
		else
		{
			Position = FbxMesh->GetControlPoints()[ControlPointsIndex];
		}

		FbxVector4 FinalPosition;
		FinalPosition = TotalMatrix.MultT(Position);
		FVector ConvertedPosition = ConvertPos(FinalPosition);

		// ensure user when this happens if attached to debugger
		if (!(ConvertedPosition.ContainsNaN() == false))
		{
			if (!bInvalidPositionFound)
			{
				bInvalidPositionFound = true;
			}

			ConvertedPosition = FVector::ZeroVector;
		}

		OutData->Points[ControlPointsIndex + ExistPointNum] = ConvertedPosition;
	}

	if (bInvalidPositionFound)
	{
		X_LOG("Invalid position (NaN or Inf) found from source position for mesh '%s'. Please verify if the source asset contains valid position. ", FbxMesh->GetName());
	}

	return true;
}

void FBXImporter::RecursiveBuildSkeleton(FbxNode* Link, std::vector<FbxNode*>& OutSortedLinks)
{
	if (IsUnrealBone(Link))
	{
		OutSortedLinks.push_back(Link);
		int32 ChildIndex;
		for (ChildIndex = 0; ChildIndex < Link->GetChildCount(); ChildIndex++)
		{
			RecursiveBuildSkeleton(Link->GetChild(ChildIndex), OutSortedLinks);
		}
	}
}

bool FBXImporter::RetrievePoseFromBindPose(const std::vector<FbxNode*>& NodeArray, FbxArray<FbxPose*> & PoseArray) const
{
	const int32 PoseCount = fbxScene->GetPoseCount();
	for (int32 PoseIndex = 0; PoseIndex < PoseCount; PoseIndex++)
	{
		FbxPose* CurrentPose = fbxScene->GetPose(PoseIndex);

		// current pose is bind pose, 
		if (CurrentPose && CurrentPose->IsBindPose())
		{
			// IsValidBindPose doesn't work reliably
			// It checks all the parent chain(regardless root given), and if the parent doesn't have correct bind pose, it fails
			// It causes more false positive issues than the real issue we have to worry about
			// If you'd like to try this, set CHECK_VALID_BIND_POSE to 1, and try the error message
			// when Autodesk fixes this bug, then we might be able to re-open this
			std::string PoseName = CurrentPose->GetName();
			// all error report status
			FbxStatus Status;

			// it does not make any difference of checking with different node
			// it is possible pose 0 -> node array 2, but isValidBindPose function returns true even with node array 0
			for (auto Current : NodeArray)
			{
				std::string  CurrentName = Current->GetName();
				NodeList pMissingAncestors, pMissingDeformers, pMissingDeformersAncestors, pWrongMatrices;

				if (CurrentPose->IsValidBindPoseVerbose(Current, pMissingAncestors, pMissingDeformers, pMissingDeformersAncestors, pWrongMatrices, 0.0001, &Status))
				{
					PoseArray.Add(CurrentPose);
					X_LOG("Valid bind pose for Pose (%s) - %s", PoseName.c_str(), Current->GetName());
					break;
				}
				else
				{
					// first try to fix up
					// add missing ancestors
					for (int i = 0; i < pMissingAncestors.GetCount(); i++)
					{
						FbxAMatrix mat = pMissingAncestors.GetAt(i)->EvaluateGlobalTransform(FBXSDK_TIME_ZERO);
						CurrentPose->Add(pMissingAncestors.GetAt(i), mat);
					}

					pMissingAncestors.Clear();
					pMissingDeformers.Clear();
					pMissingDeformersAncestors.Clear();
					pWrongMatrices.Clear();

					// check it again
					if (CurrentPose->IsValidBindPose(Current))
					{
						PoseArray.Add(CurrentPose);
						X_LOG("Valid bind pose for Pose (%s) - %s",PoseName.c_str(), Current->GetName());
						break;
					}
					else
					{
						// first try to find parent who is null group and see if you can try test it again
						FbxNode * ParentNode = Current->GetParent();
						while (ParentNode)
						{
							FbxNodeAttribute* Attr = ParentNode->GetNodeAttribute();
							if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eNull)
							{
								// found it 
								break;
							}

							// find next parent
							ParentNode = ParentNode->GetParent();
						}

						if (ParentNode && CurrentPose->IsValidBindPose(ParentNode))
						{
							PoseArray.Add(CurrentPose);
							X_LOG("Valid bind pose for Pose (%s) - %s", PoseName.c_str(), Current->GetName());
							break;
						}
						else
						{
							std::string  ErrorString = Status.GetErrorString();
							X_LOG("Not valid bind pose for Pose (%s) - Node %s : %s", PoseName.c_str(), Current->GetName());
						}
					}
				}
			}
		}
	}

	return (PoseArray.Size() > 0);
}

void FBXImporter::BuildSkeletonSystem(std::vector<FbxCluster*>& ClusterArray, std::vector<FbxNode*>& OutSortedLinks)
{
	FbxNode* Link;
	std::vector<FbxNode*> RootLinks;
	uint32 ClusterIndex;
	for (ClusterIndex = 0; ClusterIndex < ClusterArray.size(); ClusterIndex++)
	{
		Link = ClusterArray[ClusterIndex]->GetLink();
		if (Link)
		{
			Link = GetRootSkeleton(Link);
			uint32 LinkIndex;
			for (LinkIndex = 0; LinkIndex < RootLinks.size(); LinkIndex++)
			{
				if (Link == RootLinks[LinkIndex])
				{
					break;
				}
			}

			// this link is a new root, add it
			if (LinkIndex == RootLinks.size())
			{
				RootLinks.push_back(Link);
			}
		}
	}

	for (uint32 LinkIndex = 0; LinkIndex < RootLinks.size(); LinkIndex++)
	{
		RecursiveBuildSkeleton(RootLinks[LinkIndex], OutSortedLinks);
	}
}

FbxNode* FBXImporter::GetRootSkeleton(FbxNode* Link)
{
	FbxNode* RootBone = Link;

	// get Unreal skeleton root
	// mesh and dummy are used as bone if they are in the skeleton hierarchy
	while (RootBone && RootBone->GetParent())
	{
		bool bIsBlenderArmatureBone = false;
// 		if (FbxCreator == EFbxCreator::Blender)
// 		{
// 			//Hack to support armature dummy node from blender
// 			//Users do not want the null attribute node named armature which is the parent of the real root bone in blender fbx file
// 			//This is a hack since if a rigid mesh group root node is named "armature" it will be skip
// 			const FString RootBoneParentName(RootBone->GetParent()->GetName());
// 			FbxNode *GrandFather = RootBone->GetParent()->GetParent();
// 			bIsBlenderArmatureBone = (GrandFather == nullptr || GrandFather == Scene->GetRootNode()) && (RootBoneParentName.Compare(TEXT("armature"), ESearchCase::IgnoreCase) == 0);
// 		}

		FbxNodeAttribute* Attr = RootBone->GetParent()->GetNodeAttribute();
		if (Attr &&
			(Attr->GetAttributeType() == FbxNodeAttribute::eMesh ||
			(Attr->GetAttributeType() == FbxNodeAttribute::eNull && !bIsBlenderArmatureBone) ||
				Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton) &&
			RootBone->GetParent() != fbxScene->GetRootNode())
		{
			// in some case, skeletal mesh can be ancestor of bones
			// this avoids this situation
			if (Attr->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				FbxMesh* Mesh = (FbxMesh*)Attr;
				if (Mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
				{
					break;
				}
			}

			RootBone = RootBone->GetParent();
		}
		else
		{
			break;
		}
	}

	return RootBone;
}

void FBXImporter::SkinControlPointsToPose(FSkeletalMeshImportData &ImportData, FbxMesh* FbxMesh, FbxShape* FbxShape, bool bUseT0)
{
	FbxTime poseTime = FBXSDK_TIME_INFINITE;
	if (bUseT0)
	{
		poseTime = 0;
	}

	int32 VertexCount = FbxMesh->GetControlPointsCount();

	// Create a copy of the vertex array to receive vertex deformations.
	FbxVector4* VertexArray = new FbxVector4[VertexCount];

	// If a shape is provided, then it is the morphed version of the mesh.
	// So we want to deform that instead of the base mesh vertices
	if (FbxShape)
	{
		assert(FbxShape->GetControlPointsCount() == VertexCount);
		memcpy(VertexArray, FbxShape->GetControlPoints(), VertexCount * sizeof(FbxVector4));
	}
	else
	{
		memcpy(VertexArray, FbxMesh->GetControlPoints(), VertexCount * sizeof(FbxVector4));
	}


	int32 ClusterCount = 0;
	int32 SkinCount = FbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int32 i = 0; i < SkinCount; i++)
	{
		ClusterCount += ((FbxSkin *)(FbxMesh->GetDeformer(i, FbxDeformer::eSkin)))->GetClusterCount();
	}

	// Deform the vertex array with the links contained in the mesh.
	if (ClusterCount)
	{
		FbxAMatrix MeshMatrix = ComputeTotalMatrix(FbxMesh->GetNode());
		// All the links must have the same link mode.
		FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)FbxMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

		int32 i, j;
		int32 lClusterCount = 0;

		int32 lSkinCount = FbxMesh->GetDeformerCount(FbxDeformer::eSkin);

		std::vector<FbxAMatrix> ClusterDeformations;
		ClusterDeformations.resize(VertexCount);

		std::vector<double> ClusterWeights;
		ClusterWeights.resize(VertexCount);


		if (lClusterMode == FbxCluster::eAdditive)
		{
			for (i = 0; i < VertexCount; i++)
			{
				ClusterDeformations[i].SetIdentity();
			}
		}

		for (i = 0; i < lSkinCount; ++i)
		{
			lClusterCount = ((FbxSkin *)FbxMesh->GetDeformer(i, FbxDeformer::eSkin))->GetClusterCount();
			for (j = 0; j < lClusterCount; ++j)
			{
				FbxCluster* Cluster = ((FbxSkin *)FbxMesh->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);
				if (!Cluster->GetLink())
					continue;

				FbxNode* Link = Cluster->GetLink();

				FbxAMatrix lReferenceGlobalInitPosition;
				FbxAMatrix lReferenceGlobalCurrentPosition;
				FbxAMatrix lClusterGlobalInitPosition;
				FbxAMatrix lClusterGlobalCurrentPosition;
				FbxAMatrix lReferenceGeometry;
				FbxAMatrix lClusterGeometry;

				FbxAMatrix lClusterRelativeInitPosition;
				FbxAMatrix lClusterRelativeCurrentPositionInverse;
				FbxAMatrix lVertexTransformMatrix;

				if (lClusterMode == FbxCluster::eAdditive && Cluster->GetAssociateModel())
				{
					Cluster->GetTransformAssociateModelMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = fbxScene->GetAnimationEvaluator()->GetNodeGlobalTransform(Cluster->GetAssociateModel(), poseTime);
					// Geometric transform of the model
					lReferenceGeometry = GetGeometry(Cluster->GetAssociateModel());
					lReferenceGlobalCurrentPosition *= lReferenceGeometry;
				}
				else
				{
					Cluster->GetTransformMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = MeshMatrix; //pGlobalPosition;
																  // Multiply lReferenceGlobalInitPosition by Geometric Transformation
					lReferenceGeometry = GetGeometry(FbxMesh->GetNode());
					lReferenceGlobalInitPosition *= lReferenceGeometry;
				}
				// Get the link initial global position and the link current global position.
				Cluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
				lClusterGlobalCurrentPosition = Link->GetScene()->GetAnimationEvaluator()->GetNodeGlobalTransform(Link, poseTime);

				// Compute the initial position of the link relative to the reference.
				lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;

				// Compute the current position of the link relative to the reference.
				lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;

				// Compute the shift of the link relative to the reference.
				lVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;

				int32 k;
				int32 lVertexIndexCount = Cluster->GetControlPointIndicesCount();

				for (k = 0; k < lVertexIndexCount; ++k)
				{
					int32 lIndex = Cluster->GetControlPointIndices()[k];

					// Sometimes, the mesh can have less points than at the time of the skinning
					// because a smooth operator was active when skinning but has been deactivated during export.
					if (lIndex >= VertexCount)
					{
						continue;
					}

					double lWeight = Cluster->GetControlPointWeights()[k];

					if (lWeight == 0.0)
					{
						continue;
					}

					// Compute the influence of the link on the vertex.
					FbxAMatrix lInfluence = lVertexTransformMatrix;
					MatrixScale(lInfluence, lWeight);

					if (lClusterMode == FbxCluster::eAdditive)
					{
						// Multiply with to the product of the deformations on the vertex.
						MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
						ClusterDeformations[lIndex] = lInfluence * ClusterDeformations[lIndex];

						// Set the link to 1.0 just to know this vertex is influenced by a link.
						ClusterWeights[lIndex] = 1.0;
					}
					else // lLinkMode == KFbxLink::eNORMALIZE || lLinkMode == KFbxLink::eTOTAL1
					{
						// Add to the sum of the deformations on the vertex.
						MatrixAdd(ClusterDeformations[lIndex], lInfluence);

						// Add to the sum of weights to either normalize or complete the vertex.
						ClusterWeights[lIndex] += lWeight;
					}

				}
			}
		}

		for (i = 0; i < VertexCount; i++)
		{
			FbxVector4 lSrcVertex = VertexArray[i];
			FbxVector4& lDstVertex = VertexArray[i];
			double lWeight = ClusterWeights[i];

			// Deform the vertex if there was at least a link with an influence on the vertex,
			if (lWeight != 0.0)
			{
				lDstVertex = ClusterDeformations[i].MultT(lSrcVertex);

				if (lClusterMode == FbxCluster::eNormalize)
				{
					// In the normalized link mode, a vertex is always totally influenced by the links. 
					lDstVertex /= lWeight;
				}
				else if (lClusterMode == FbxCluster::eTotalOne)
				{
					// In the total 1 link mode, a vertex can be partially influenced by the links. 
					lSrcVertex *= (1.0 - lWeight);
					lDstVertex += lSrcVertex;
				}
			}
		}

		// change the vertex position
		int32 ExistPointNum = ImportData.Points.size();
		int32 StartPointIndex = ExistPointNum - VertexCount;
		for (int32 ControlPointsIndex = 0; ControlPointsIndex < VertexCount; ControlPointsIndex++)
		{
			ImportData.Points[ControlPointsIndex + StartPointIndex] = ConvertPos(MeshMatrix.MultT(VertexArray[ControlPointsIndex]));
		}

	}

	delete[] VertexArray;
}

FbxAMatrix FBXImporter::ComputeTotalMatrix(FbxNode* Node)
{
	FbxAMatrix Geometry;
	FbxVector4 Translation, Rotation, Scaling;
	Translation = Node->GetGeometricTranslation(FbxNode::eSourcePivot);
	Rotation = Node->GetGeometricRotation(FbxNode::eSourcePivot);
	Scaling = Node->GetGeometricScaling(FbxNode::eSourcePivot);
	Geometry.SetT(Translation);
	Geometry.SetR(Rotation);
	Geometry.SetS(Scaling);

	//For Single Matrix situation, obtain transfrom matrix from eDESTINATION_SET, which include pivot offsets and pre/post rotations.
	FbxAMatrix& GlobalTransform = fbxScene->GetAnimationEvaluator()->GetNodeGlobalTransform(Node);

	//We can bake the pivot only if we don't transform the vertex to the absolute position
	if (!ImportOptions->bTransformVertexToAbsolute)
	{
		if (ImportOptions->bBakePivotInVertex)
		{
			FbxAMatrix PivotGeometry;
			FbxVector4 RotationPivot = Node->GetRotationPivot(FbxNode::eSourcePivot);
			FbxVector4 FullPivot;
			FullPivot[0] = -RotationPivot[0];
			FullPivot[1] = -RotationPivot[1];
			FullPivot[2] = -RotationPivot[2];
			PivotGeometry.SetT(FullPivot);
			Geometry = Geometry * PivotGeometry;
		}
		else
		{
			//No Vertex transform and no bake pivot, it will be the mesh as-is.
			Geometry.SetIdentity();
		}
	}
	//We must always add the geometric transform. Only Max use the geometric transform which is an offset to the local transform of the node
	FbxAMatrix TotalMatrix = ImportOptions->bTransformVertexToAbsolute ? GlobalTransform * Geometry : Geometry;

	return TotalMatrix;
}

FbxAMatrix FBXImporter::ComputeSkeletalMeshTotalMatrix(FbxNode* Node, FbxNode *RootSkeletalNode)
{
	if (ImportOptions->bImportScene && !ImportOptions->bTransformVertexToAbsolute && RootSkeletalNode != nullptr && RootSkeletalNode != Node)
	{
		FbxAMatrix GlobalTransform = fbxScene->GetAnimationEvaluator()->GetNodeGlobalTransform(Node);
		FbxAMatrix GlobalSkeletalMeshRootTransform = fbxScene->GetAnimationEvaluator()->GetNodeGlobalTransform(RootSkeletalNode);
		FbxAMatrix TotalMatrix = GlobalSkeletalMeshRootTransform.Inverse() * GlobalTransform;
		return TotalMatrix;
	}
	return ComputeTotalMatrix(Node);
}

bool FBXImporter::IsOddNegativeScale(FbxAMatrix& TotalMatrix)
{
	FbxVector4 Scale = TotalMatrix.GetS();
	int32 NegativeNum = 0;

	if (Scale[0] < 0) NegativeNum++;
	if (Scale[1] < 0) NegativeNum++;
	if (Scale[2] < 0) NegativeNum++;

	return NegativeNum == 1 || NegativeNum == 3;
}

class IMeshBuildData
{
public:
	virtual ~IMeshBuildData() { }

	virtual uint32 GetWedgeIndex(uint32 FaceIndex, uint32 TriIndex) = 0;
	virtual uint32 GetVertexIndex(uint32 WedgeIndex) = 0;
	virtual uint32 GetVertexIndex(uint32 FaceIndex, uint32 TriIndex) = 0;
	virtual FVector GetVertexPosition(uint32 WedgeIndex) = 0;
	virtual FVector GetVertexPosition(uint32 FaceIndex, uint32 TriIndex) = 0;
	virtual Vector2 GetVertexUV(uint32 FaceIndex, uint32 TriIndex, uint32 UVIndex) = 0;
	virtual uint32 GetFaceSmoothingGroups(uint32 FaceIndex) = 0;

	virtual uint32 GetNumFaces() = 0;
	virtual uint32 GetNumWedges() = 0;

	virtual std::vector<FVector>& GetTangentArray(uint32 Axis) = 0;
	virtual void ValidateTangentArraySize() = 0;

	virtual SMikkTSpaceInterface* GetMikkTInterface() = 0;
	virtual void* GetMikkTUserData() = 0;

	const MeshBuildOptions& BuildOptions;
	std::vector<std::string>* OutWarningMessages;
	std::vector<std::string>* OutWarningNames;
	bool bTooManyVerts;

protected:
	IMeshBuildData(
		const MeshBuildOptions& InBuildOptions,
		std::vector<std::string>* InWarningMessages,
		std::vector<std::string>* InWarningNames)
		: BuildOptions(InBuildOptions)
		, OutWarningMessages(InWarningMessages)
		, OutWarningNames(InWarningNames)
		, bTooManyVerts(false)
	{
	}
};
/**
* Smoothing group interpretation helper structure.
*/
struct FFanFace
{
	int32 FaceIndex;
	int32 LinkedVertexIndex;
	bool bFilled;
	bool bBlendTangents;
	bool bBlendNormals;
};

// MikkTSpace implementations for skeletal meshes, where tangents/bitangents are ultimately derived from lists of attributes.

// Holder for skeletal data to be passed to MikkTSpace.
// Holds references to the wedge, face and points vectors that BuildSkeletalMesh is given.
// Holds reference to the calculated normals array, which will be fleshed out if they've been calculated.
// Holds reference to the newly created tangent and bitangent arrays, which MikkTSpace will fleshed out if required.
class MikkTSpace_Skeletal_Mesh
{
public:
	const std::vector<FMeshWedge>	&wedges;			//Reference to wedge list.
	const std::vector<FMeshFace>		&faces;				//Reference to face list.	Also contains normal/tangent/bitanget/UV coords for each vertex of the face.
	const std::vector<FVector>		&points;			//Reference to position list.
	bool						bComputeNormals;	//Copy of bComputeNormals.
	std::vector<FVector>				&TangentsX;			//Reference to newly created tangents list.
	std::vector<FVector>				&TangentsY;			//Reference to newly created bitangents list.
	std::vector<FVector>				&TangentsZ;			//Reference to computed normals, will be empty otherwise.

	MikkTSpace_Skeletal_Mesh(
		const std::vector<FMeshWedge>	&Wedges,
		const std::vector<FMeshFace>		&Faces,
		const std::vector<FVector>		&Points,
		bool						bInComputeNormals,
		std::vector<FVector>				&VertexTangentsX,
		std::vector<FVector>				&VertexTangentsY,
		std::vector<FVector>				&VertexTangentsZ
	)
		:
		wedges(Wedges),
		faces(Faces),
		points(Points),
		bComputeNormals(bInComputeNormals),
		TangentsX(VertexTangentsX),
		TangentsY(VertexTangentsY),
		TangentsZ(VertexTangentsZ)
	{
	}
};

static int MikkGetNumFaces_Skeletal(const SMikkTSpaceContext* Context)
{
	MikkTSpace_Skeletal_Mesh *UserData = (MikkTSpace_Skeletal_Mesh*)(Context->m_pUserData);
	return (int)UserData->faces.size();
}

static int MikkGetNumVertsOfFace_Skeletal(const SMikkTSpaceContext* Context, const int FaceIdx)
{
	// Confirmed?
	return 3;
}

static void MikkGetPosition_Skeletal(const SMikkTSpaceContext* Context, float Position[3], const int FaceIdx, const int VertIdx)
{
	MikkTSpace_Skeletal_Mesh *UserData = (MikkTSpace_Skeletal_Mesh*)(Context->m_pUserData);
	const FVector &VertexPosition = UserData->points[UserData->wedges[UserData->faces[FaceIdx].iWedge[VertIdx]].iVertex];
	Position[0] = VertexPosition.X;
	Position[1] = VertexPosition.Y;
	Position[2] = VertexPosition.Z;
}

static void MikkGetNormal_Skeletal(const SMikkTSpaceContext* Context, float Normal[3], const int FaceIdx, const int VertIdx)
{
	MikkTSpace_Skeletal_Mesh *UserData = (MikkTSpace_Skeletal_Mesh*)(Context->m_pUserData);
	// Get different normals depending on whether they've been calculated or not.
	if (UserData->bComputeNormals) {
		FVector &VertexNormal = UserData->TangentsZ[FaceIdx * 3 + VertIdx];
		Normal[0] = VertexNormal.X;
		Normal[1] = VertexNormal.Y;
		Normal[2] = VertexNormal.Z;
	}
	else
	{
		const FVector &VertexNormal = UserData->faces[FaceIdx].TangentZ[VertIdx];
		Normal[0] = VertexNormal.X;
		Normal[1] = VertexNormal.Y;
		Normal[2] = VertexNormal.Z;
	}
}

static void MikkSetTSpaceBasic_Skeletal(const SMikkTSpaceContext* Context, const float Tangent[3], const float BitangentSign, const int FaceIdx, const int VertIdx)
{
	MikkTSpace_Skeletal_Mesh *UserData = (MikkTSpace_Skeletal_Mesh*)(Context->m_pUserData);
	FVector &VertexTangent = UserData->TangentsX[FaceIdx * 3 + VertIdx];
	VertexTangent.X = Tangent[0];
	VertexTangent.Y = Tangent[1];
	VertexTangent.Z = Tangent[2];

	FVector Bitangent;
	// Get different normals depending on whether they've been calculated or not.
	if (UserData->bComputeNormals) {
		Bitangent = BitangentSign * FVector::CrossProduct(UserData->TangentsZ[FaceIdx * 3 + VertIdx], VertexTangent);
	}
	else
	{
		Bitangent = BitangentSign * FVector::CrossProduct(UserData->faces[FaceIdx].TangentZ[VertIdx], VertexTangent);
	}
	FVector &VertexBitangent = UserData->TangentsY[FaceIdx * 3 + VertIdx];
	// Switch the tangent space swizzle to X+Y-Z+ for legacy reasons.
	VertexBitangent.X = -Bitangent[0];
	VertexBitangent.Y = -Bitangent[1];
	VertexBitangent.Z = -Bitangent[2];
}

static void MikkGetTexCoord_Skeletal(const SMikkTSpaceContext* Context, float UV[2], const int FaceIdx, const int VertIdx)
{
	MikkTSpace_Skeletal_Mesh *UserData = (MikkTSpace_Skeletal_Mesh*)(Context->m_pUserData);
	const Vector2 &TexCoord = UserData->wedges[UserData->faces[FaceIdx].iWedge[VertIdx]].UVs[0];
	UV[0] = TexCoord.X;
	UV[1] = TexCoord.Y;
}




class SkeletalMeshBuildData final : public IMeshBuildData
{
public:
	SkeletalMeshBuildData(
		FSkeletalMeshLODModel& InLODModel,
		const FReferenceSkeleton& InRefSkeleton,
		const std::vector<FVertInfluence>& InInfluences,
		const std::vector<FMeshWedge>& InWedges,
		const std::vector<FMeshFace>& InFaces,
		const std::vector<FVector>& InPoints,
		const std::vector<int32>& InPointToOriginalMap,
		const MeshBuildOptions& InBuildOptions,
		std::vector<std::string>* InWarningMessages,
		std::vector<std::string>* InWarningNames)
		: IMeshBuildData(InBuildOptions, InWarningMessages, InWarningNames)
		, MikkTUserData(InWedges, InFaces, InPoints, InBuildOptions.bComputeNormals, TangentX, TangentY, TangentZ)
		, LODModel(InLODModel)
		, RefSkeleton(InRefSkeleton)
		, Influences(InInfluences)
		, Wedges(InWedges)
		, Faces(InFaces)
		, Points(InPoints)
		, PointToOriginalMap(InPointToOriginalMap)
	{
		MikkTInterface.m_getNormal = MikkGetNormal_Skeletal;
		MikkTInterface.m_getNumFaces = MikkGetNumFaces_Skeletal;
		MikkTInterface.m_getNumVerticesOfFace = MikkGetNumVertsOfFace_Skeletal;
		MikkTInterface.m_getPosition = MikkGetPosition_Skeletal;
		MikkTInterface.m_getTexCoord = MikkGetTexCoord_Skeletal;
		MikkTInterface.m_setTSpaceBasic = MikkSetTSpaceBasic_Skeletal;
		MikkTInterface.m_setTSpace = nullptr;

		//Fill the NTBs information
		if (!InBuildOptions.bComputeNormals || !InBuildOptions.bComputeTangents)
		{
			if (!InBuildOptions.bComputeTangents)
			{
				TangentX.resize(Wedges.size());
				TangentY.resize(Wedges.size());
			}

			if (!InBuildOptions.bComputeNormals)
			{
				TangentZ.resize(Wedges.size());
			}

			for (const FMeshFace& MeshFace : Faces)
			{
				for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
				{
					uint32 WedgeIndex = MeshFace.iWedge[CornerIndex];
					if (!InBuildOptions.bComputeTangents)
					{
						TangentX[WedgeIndex] = MeshFace.TangentX[CornerIndex];
						TangentY[WedgeIndex] = MeshFace.TangentY[CornerIndex];
					}
					if (!InBuildOptions.bComputeNormals)
					{
						TangentZ[WedgeIndex] = MeshFace.TangentZ[CornerIndex];
					}
				}
			}
		}
	}

	virtual uint32 GetWedgeIndex(uint32 FaceIndex, uint32 TriIndex) override
	{
		return Faces[FaceIndex].iWedge[TriIndex];
	}

	virtual uint32 GetVertexIndex(uint32 WedgeIndex) override
	{
		return Wedges[WedgeIndex].iVertex;
	}

	virtual uint32 GetVertexIndex(uint32 FaceIndex, uint32 TriIndex) override
	{
		return Wedges[Faces[FaceIndex].iWedge[TriIndex]].iVertex;
	}

	virtual FVector GetVertexPosition(uint32 WedgeIndex) override
	{
		return Points[Wedges[WedgeIndex].iVertex];
	}

	virtual FVector GetVertexPosition(uint32 FaceIndex, uint32 TriIndex) override
	{
		return Points[Wedges[Faces[FaceIndex].iWedge[TriIndex]].iVertex];
	}

	virtual Vector2 GetVertexUV(uint32 FaceIndex, uint32 TriIndex, uint32 UVIndex) override
	{
		return Wedges[Faces[FaceIndex].iWedge[TriIndex]].UVs[UVIndex];
	}

	virtual uint32 GetFaceSmoothingGroups(uint32 FaceIndex)
	{
		return Faces[FaceIndex].SmoothingGroups;
	}

	virtual uint32 GetNumFaces() override
	{
		return Faces.size();
	}

	virtual uint32 GetNumWedges() override
	{
		return Wedges.size();
	}

	virtual std::vector<FVector>& GetTangentArray(uint32 Axis) override
	{
		if (Axis == 0)
		{
			return TangentX;
		}
		else if (Axis == 1)
		{
			return TangentY;
		}

		return TangentZ;
	}

	virtual void ValidateTangentArraySize() override
	{
		assert(TangentX.size() == Wedges.size());
		assert(TangentY.size() == Wedges.size());
		assert(TangentZ.size() == Wedges.size());
	}

	virtual SMikkTSpaceInterface* GetMikkTInterface() override
	{
		return &MikkTInterface;
	}

	virtual void* GetMikkTUserData() override
	{
		return (void*)&MikkTUserData;
	}

	std::vector<FVector> TangentX;
	std::vector<FVector> TangentY;
	std::vector<FVector> TangentZ;
	std::vector<FSkinnedMeshChunk*> Chunks;

	SMikkTSpaceInterface MikkTInterface;
	MikkTSpace_Skeletal_Mesh MikkTUserData;

	FSkeletalMeshLODModel& LODModel;
	const FReferenceSkeleton& RefSkeleton;
	const std::vector<FVertInfluence>& Influences;
	const std::vector<FMeshWedge>& Wedges;
	const std::vector<FMeshFace>& Faces;
	const std::vector<FVector>& Points;
	const std::vector<int32>& PointToOriginalMap;
};
/**
* Returns true if the specified normal vectors are about equal
*/
inline bool NormalsEqual(const FVector& V1, const FVector& V2)
{
	const float Epsilon = THRESH_NORMALS_ARE_SAME;
	return FMath::Abs(V1.X - V2.X) <= Epsilon && FMath::Abs(V1.Y - V2.Y) <= Epsilon && FMath::Abs(V1.Z - V2.Z) <= Epsilon;
}

/**
* Returns true if the specified points are about equal
*/
inline bool PointsEqual(const FVector& V1, const FVector& V2, float ComparisonThreshold)
{
	if (FMath::Abs(V1.X - V2.X) > ComparisonThreshold
		|| FMath::Abs(V1.Y - V2.Y) > ComparisonThreshold
		|| FMath::Abs(V1.Z - V2.Z) > ComparisonThreshold)
	{
		return false;
	}
	return true;
}
inline bool PointsEqual(const FVector& V1, const FVector& V2, const FOverlappingThresholds& OverlappingThreshold)
{
	const float Epsilon = OverlappingThreshold.ThresholdPosition;
	return FMath::Abs(V1.X - V2.X) <= Epsilon && FMath::Abs(V1.Y - V2.Y) <= Epsilon && FMath::Abs(V1.Z - V2.Z) <= Epsilon;
}
/**
* Returns true if the specified points are about equal
*/
inline bool PointsEqual(const FVector& V1, const FVector& V2, bool bUseEpsilonCompare = true)
{
	const float Epsilon = bUseEpsilonCompare ? THRESH_POINTS_ARE_SAME : 0.0f;
	return FMath::Abs(V1.X - V2.X) <= Epsilon && FMath::Abs(V1.Y - V2.Y) <= Epsilon && FMath::Abs(V1.Z - V2.Z) <= Epsilon;
}
inline bool NormalsEqual(const FVector& V1, const FVector& V2, const FOverlappingThresholds& OverlappingThreshold)
{
	const float Epsilon = OverlappingThreshold.ThresholdTangentNormal;
	return FMath::Abs(V1.X - V2.X) <= Epsilon && FMath::Abs(V1.Y - V2.Y) <= Epsilon && FMath::Abs(V1.Z - V2.Z) <= Epsilon;
}

inline bool UVsEqual(const Vector2& V1, const Vector2& V2)
{
	const float Epsilon = 1.0f / 1024.0f;
	return FMath::Abs(V1.X - V2.X) <= Epsilon && FMath::Abs(V1.Y - V2.Y) <= Epsilon;
}
inline bool UVsEqual(const Vector2& V1, const Vector2& V2, const FOverlappingThresholds& OverlappingThreshold)
{
	const float Epsilon = OverlappingThreshold.ThresholdUV;
	return FMath::Abs(V1.X - V2.X) <= Epsilon && FMath::Abs(V1.Y - V2.Y) <= Epsilon;
}


/** Helper struct for building acceleration structures. */
struct FIndexAndZ
{
	float Z;
	int32 Index;

	/** Default constructor. */
	FIndexAndZ() {}

	/** Initialization constructor. */
	FIndexAndZ(int32 InIndex, FVector V)
	{
		Z = 0.30f * V.X + 0.33f * V.Y + 0.37f * V.Z;
		Index = InIndex;
	}
};
/** Sorting function for vertex Z/index pairs. */
struct FCompareIndexAndZ
{
	inline bool operator()(FIndexAndZ const& A, FIndexAndZ const& B) const { return A.Z < B.Z; }
};

class FSkeletalMeshUtilityBuilder
{
public:
	FSkeletalMeshUtilityBuilder()
		: Stage(EStage::Uninit)
	{
	}

public:
	void Skeletal_FindOverlappingCorners(
		FOverlappingCorners& OutOverlappingCorners,
		IMeshBuildData* BuildData,
		float ComparisonThreshold
	)
	{
		int32 NumFaces = BuildData->GetNumFaces();
		int32 NumWedges = BuildData->GetNumWedges();
		assert(NumFaces * 3 <= NumWedges);

		// Create a list of vertex Z/index pairs
		std::vector<FIndexAndZ> VertIndexAndZ;
		VertIndexAndZ.clear();
		for (int32 FaceIndex = 0; FaceIndex < NumFaces; FaceIndex++)
		{
			for (int32 TriIndex = 0; TriIndex < 3; ++TriIndex)
			{
				uint32 Index = BuildData->GetWedgeIndex(FaceIndex, TriIndex);
				VertIndexAndZ.emplace_back(FIndexAndZ(Index, BuildData->GetVertexPosition(Index)));
			}
		}

		// Sort the vertices by z value
		//VertIndexAndZ.Sort(FCompareIndexAndZ());
		std::sort(VertIndexAndZ.begin(), VertIndexAndZ.end(), FCompareIndexAndZ());

		OutOverlappingCorners.Init(NumWedges);

		// Search for duplicates, quickly!
		for (uint32 i = 0; i < VertIndexAndZ.size(); i++)
		{
			// only need to search forward, since we add pairs both ways
			for (uint32 j = i + 1; j < VertIndexAndZ.size(); j++)
			{
				if (FMath::Abs(VertIndexAndZ[j].Z - VertIndexAndZ[i].Z) > ComparisonThreshold)
					break; // can't be any more dups

				FVector PositionA = BuildData->GetVertexPosition(VertIndexAndZ[i].Index);
				FVector PositionB = BuildData->GetVertexPosition(VertIndexAndZ[j].Index);

				if (PointsEqual(PositionA, PositionB, ComparisonThreshold))
				{
					OutOverlappingCorners.Add(VertIndexAndZ[i].Index, VertIndexAndZ[j].Index);
				}
			}
		}

		OutOverlappingCorners.FinishAdding();
	}

	void Skeletal_ComputeTriangleTangents(
		std::vector<FVector>& TriangleTangentX,
		std::vector<FVector>& TriangleTangentY,
		std::vector<FVector>& TriangleTangentZ,
		IMeshBuildData* BuildData,
		float ComparisonThreshold
	)
	{
		int32 NumTriangles = BuildData->GetNumFaces();
		TriangleTangentX.clear();
		TriangleTangentY.clear();
		TriangleTangentZ.clear();

		for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; TriangleIndex++)
		{
			const int32 UVIndex = 0;
			FVector P[3];

			for (int32 i = 0; i < 3; ++i)
			{
				P[i] = BuildData->GetVertexPosition(TriangleIndex, i);
			}

			const FVector Normal = ((P[1] - P[2]) ^ (P[0] - P[2])).GetSafeNormal(ComparisonThreshold);
			FMatrix	ParameterToLocal(
				FPlane(P[1].X - P[0].X, P[1].Y - P[0].Y, P[1].Z - P[0].Z, 0),
				FPlane(P[2].X - P[0].X, P[2].Y - P[0].Y, P[2].Z - P[0].Z, 0),
				FPlane(P[0].X, P[0].Y, P[0].Z, 0),
				FPlane(0, 0, 0, 1)
			);

			Vector2 T1 = BuildData->GetVertexUV(TriangleIndex, 0, UVIndex);
			Vector2 T2 = BuildData->GetVertexUV(TriangleIndex, 1, UVIndex);
			Vector2 T3 = BuildData->GetVertexUV(TriangleIndex, 2, UVIndex);
			FMatrix ParameterToTexture(
				FPlane(T2.X - T1.X, T2.Y - T1.Y, 0, 0),
				FPlane(T3.X - T1.X, T3.Y - T1.Y, 0, 0),
				FPlane(T1.X, T1.Y, 1, 0),
				FPlane(0, 0, 0, 1)
			);

			// Use InverseSlow to catch singular matrices.  Inverse can miss this sometimes.
			const FMatrix TextureToLocal = ParameterToTexture.Inverse() * ParameterToLocal;

			TriangleTangentX.push_back(TextureToLocal.TransformVector(FVector(1, 0, 0)).GetSafeNormal());
			TriangleTangentY.push_back(TextureToLocal.TransformVector(FVector(0, 1, 0)).GetSafeNormal());
			TriangleTangentZ.push_back(Normal);

			FVector::CreateOrthonormalBasis(
				TriangleTangentX[TriangleIndex],
				TriangleTangentY[TriangleIndex],
				TriangleTangentZ[TriangleIndex]
			);
		}
	}

	void Skeletal_ComputeTangents(
		IMeshBuildData* BuildData,
		const FOverlappingCorners& OverlappingCorners
	)
	{
		bool bBlendOverlappingNormals = true;
		bool bIgnoreDegenerateTriangles = BuildData->BuildOptions.bRemoveDegenerateTriangles;

		// Compute per-triangle tangents.
		std::vector<FVector> TriangleTangentX;
		std::vector<FVector> TriangleTangentY;
		std::vector<FVector> TriangleTangentZ;

		Skeletal_ComputeTriangleTangents(
			TriangleTangentX,
			TriangleTangentY,
			TriangleTangentZ,
			BuildData,
			bIgnoreDegenerateTriangles ? SMALL_NUMBER : 0.0f
		);

		std::vector<FVector>& WedgeTangentX = BuildData->GetTangentArray(0);
		std::vector<FVector>& WedgeTangentY = BuildData->GetTangentArray(1);
		std::vector<FVector>& WedgeTangentZ = BuildData->GetTangentArray(2);

		// Declare these out here to avoid reallocations.
		std::vector<FFanFace> RelevantFacesForCorner[3];
		std::vector<int32> AdjacentFaces;

		int32 NumFaces = BuildData->GetNumFaces();
		int32 NumWedges = BuildData->GetNumWedges();
		assert(NumFaces * 3 <= NumWedges);

		// Allocate storage for tangents if none were provided.
		if (WedgeTangentX.size() != NumWedges)
		{
			WedgeTangentX.clear();
			WedgeTangentX.resize(NumWedges);
		}
		if (WedgeTangentY.size() != NumWedges)
		{
			WedgeTangentY.clear();
			WedgeTangentY.resize(NumWedges);
		}
		if (WedgeTangentZ.size() != NumWedges)
		{
			WedgeTangentZ.clear();
			WedgeTangentZ.resize(NumWedges);
		}

		for (int32 FaceIndex = 0; FaceIndex < NumFaces; FaceIndex++)
		{
			int32 WedgeOffset = FaceIndex * 3;
			FVector CornerPositions[3];
			FVector CornerTangentX[3];
			FVector CornerTangentY[3];
			FVector CornerTangentZ[3];

			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				CornerTangentX[CornerIndex] = FVector::ZeroVector;
				CornerTangentY[CornerIndex] = FVector::ZeroVector;
				CornerTangentZ[CornerIndex] = FVector::ZeroVector;
				CornerPositions[CornerIndex] = BuildData->GetVertexPosition(FaceIndex, CornerIndex);
				RelevantFacesForCorner[CornerIndex].clear();
			}

			// Don't process degenerate triangles.
			if (PointsEqual(CornerPositions[0], CornerPositions[1], BuildData->BuildOptions.OverlappingThresholds)
				|| PointsEqual(CornerPositions[0], CornerPositions[2], BuildData->BuildOptions.OverlappingThresholds)
				|| PointsEqual(CornerPositions[1], CornerPositions[2], BuildData->BuildOptions.OverlappingThresholds))
			{
				continue;
			}

			// No need to process triangles if tangents already exist.
			bool bCornerHasTangents[3] = { 0 };
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				bCornerHasTangents[CornerIndex] = !WedgeTangentX[WedgeOffset + CornerIndex].IsZero()
					&& !WedgeTangentY[WedgeOffset + CornerIndex].IsZero()
					&& !WedgeTangentZ[WedgeOffset + CornerIndex].IsZero();
			}
			if (bCornerHasTangents[0] && bCornerHasTangents[1] && bCornerHasTangents[2])
			{
				continue;
			}

			// Calculate smooth vertex normals.
			float Determinant = FVector::Triple(
				TriangleTangentX[FaceIndex],
				TriangleTangentY[FaceIndex],
				TriangleTangentZ[FaceIndex]
			);

			// Start building a list of faces adjacent to this face.
			AdjacentFaces.clear();
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				int32 ThisCornerIndex = WedgeOffset + CornerIndex;
				const std::vector<int32>& DupVerts = OverlappingCorners.FindIfOverlapping(ThisCornerIndex);
				if (DupVerts.size() == 0)
				{
					AddUnique(AdjacentFaces,ThisCornerIndex / 3); // I am a "dup" of myself
				}
				for (uint32 k = 0; k < DupVerts.size(); k++)
				{
					AddUnique(AdjacentFaces, DupVerts[k] / 3);
				}
			}

			// We need to sort these here because the criteria for point equality is
			// exact, so we must ensure the exact same order for all dups.
			//AdjacentFaces.Sort();
			std::sort(AdjacentFaces.begin(), AdjacentFaces.end());

			// Process adjacent faces
			for (uint32 AdjacentFaceIndex = 0; AdjacentFaceIndex < AdjacentFaces.size(); AdjacentFaceIndex++)
			{
				int32 OtherFaceIndex = AdjacentFaces[AdjacentFaceIndex];
				for (int32 OurCornerIndex = 0; OurCornerIndex < 3; OurCornerIndex++)
				{
					if (bCornerHasTangents[OurCornerIndex])
						continue;

					FFanFace NewFanFace;
					int32 CommonIndexCount = 0;

					// Check for vertices in common.
					if (FaceIndex == OtherFaceIndex)
					{
						CommonIndexCount = 3;
						NewFanFace.LinkedVertexIndex = OurCornerIndex;
					}
					else
					{
						// Check matching vertices against main vertex .
						for (int32 OtherCornerIndex = 0; OtherCornerIndex < 3; OtherCornerIndex++)
						{
							if (PointsEqual(
								CornerPositions[OurCornerIndex],
								BuildData->GetVertexPosition(OtherFaceIndex, OtherCornerIndex),
								BuildData->BuildOptions.OverlappingThresholds
							))
							{
								CommonIndexCount++;
								NewFanFace.LinkedVertexIndex = OtherCornerIndex;
							}
						}
					}

					// Add if connected by at least one point. Smoothing matches are considered later.
					if (CommonIndexCount > 0)
					{
						NewFanFace.FaceIndex = OtherFaceIndex;
						NewFanFace.bFilled = (OtherFaceIndex == FaceIndex); // Starter face for smoothing floodfill.
						NewFanFace.bBlendTangents = NewFanFace.bFilled;
						NewFanFace.bBlendNormals = NewFanFace.bFilled;
						RelevantFacesForCorner[OurCornerIndex].push_back(NewFanFace);
					}
				}
			}

			// Find true relevance of faces for a vertex normal by traversing
			// smoothing-group-compatible connected triangle fans around common vertices.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				if (bCornerHasTangents[CornerIndex])
					continue;

				int32 NewConnections;
				do
				{
					NewConnections = 0;
					for (uint32 OtherFaceIdx = 0; OtherFaceIdx < RelevantFacesForCorner[CornerIndex].size(); OtherFaceIdx++)
					{
						FFanFace& OtherFace = RelevantFacesForCorner[CornerIndex][OtherFaceIdx];
						// The vertex' own face is initially the only face with bFilled == true.
						if (OtherFace.bFilled)
						{
							for (uint32 NextFaceIndex = 0; NextFaceIndex < RelevantFacesForCorner[CornerIndex].size(); NextFaceIndex++)
							{
								FFanFace& NextFace = RelevantFacesForCorner[CornerIndex][NextFaceIndex];
								if (!NextFace.bFilled) // && !NextFace.bBlendTangents)
								{
									if (NextFaceIndex != OtherFaceIdx)
										//&& (BuildData->GetFaceSmoothingGroups(NextFace.FaceIndex) & BuildData->GetFaceSmoothingGroups(OtherFace.FaceIndex)))
									{
										int32 CommonVertices = 0;
										int32 CommonTangentVertices = 0;
										int32 CommonNormalVertices = 0;
										for (int32 OtherCornerIndex = 0; OtherCornerIndex < 3; OtherCornerIndex++)
										{
											for (int32 NextCornerIndex = 0; NextCornerIndex < 3; NextCornerIndex++)
											{
												int32 NextVertexIndex = BuildData->GetVertexIndex(NextFace.FaceIndex, NextCornerIndex);
												int32 OtherVertexIndex = BuildData->GetVertexIndex(OtherFace.FaceIndex, OtherCornerIndex);
												if (PointsEqual(
													BuildData->GetVertexPosition(NextFace.FaceIndex, NextCornerIndex),
													BuildData->GetVertexPosition(OtherFace.FaceIndex, OtherCornerIndex),
													BuildData->BuildOptions.OverlappingThresholds))
												{
													CommonVertices++;


													if (UVsEqual(
														BuildData->GetVertexUV(NextFace.FaceIndex, NextCornerIndex, 0),
														BuildData->GetVertexUV(OtherFace.FaceIndex, OtherCornerIndex, 0),
														BuildData->BuildOptions.OverlappingThresholds))
													{
														CommonTangentVertices++;
													}
													if (bBlendOverlappingNormals
														|| NextVertexIndex == OtherVertexIndex)
													{
														CommonNormalVertices++;
													}
												}
											}
										}
										// Flood fill faces with more than one common vertices which must be touching edges.
										if (CommonVertices > 1)
										{
											NextFace.bFilled = true;
											NextFace.bBlendNormals = (CommonNormalVertices > 1);
											NewConnections++;

											// Only blend tangents if there is no UV seam along the edge with this face.
											if (OtherFace.bBlendTangents && CommonTangentVertices > 1)
											{
												float OtherDeterminant = FVector::Triple(
													TriangleTangentX[NextFace.FaceIndex],
													TriangleTangentY[NextFace.FaceIndex],
													TriangleTangentZ[NextFace.FaceIndex]
												);
												if ((Determinant * OtherDeterminant) > 0.0f)
												{
													NextFace.bBlendTangents = true;
												}
											}
										}
									}
								}
							}
						}
					}
				} while (NewConnections > 0);
			}

			// Vertex normal construction.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				if (bCornerHasTangents[CornerIndex])
				{
					CornerTangentX[CornerIndex] = WedgeTangentX[WedgeOffset + CornerIndex];
					CornerTangentY[CornerIndex] = WedgeTangentY[WedgeOffset + CornerIndex];
					CornerTangentZ[CornerIndex] = WedgeTangentZ[WedgeOffset + CornerIndex];
				}
				else
				{
					for (uint32 RelevantFaceIdx = 0; RelevantFaceIdx < RelevantFacesForCorner[CornerIndex].size(); RelevantFaceIdx++)
					{
						FFanFace const& RelevantFace = RelevantFacesForCorner[CornerIndex][RelevantFaceIdx];
						if (RelevantFace.bFilled)
						{
							int32 OtherFaceIndex = RelevantFace.FaceIndex;
							if (RelevantFace.bBlendTangents)
							{
								CornerTangentX[CornerIndex] += TriangleTangentX[OtherFaceIndex];
								CornerTangentY[CornerIndex] += TriangleTangentY[OtherFaceIndex];
							}
							if (RelevantFace.bBlendNormals)
							{
								CornerTangentZ[CornerIndex] += TriangleTangentZ[OtherFaceIndex];
							}
						}
					}
					if (!WedgeTangentX[WedgeOffset + CornerIndex].IsZero())
					{
						CornerTangentX[CornerIndex] = WedgeTangentX[WedgeOffset + CornerIndex];
					}
					if (!WedgeTangentY[WedgeOffset + CornerIndex].IsZero())
					{
						CornerTangentY[CornerIndex] = WedgeTangentY[WedgeOffset + CornerIndex];
					}
					if (!WedgeTangentZ[WedgeOffset + CornerIndex].IsZero())
					{
						CornerTangentZ[CornerIndex] = WedgeTangentZ[WedgeOffset + CornerIndex];
					}
				}
			}

			// Normalization.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				CornerTangentX[CornerIndex].Normalize();
				CornerTangentY[CornerIndex].Normalize();
				CornerTangentZ[CornerIndex].Normalize();

				// Gram-Schmidt orthogonalization
				CornerTangentY[CornerIndex] -= CornerTangentX[CornerIndex] * (CornerTangentX[CornerIndex] | CornerTangentY[CornerIndex]);
				CornerTangentY[CornerIndex].Normalize();

				CornerTangentX[CornerIndex] -= CornerTangentZ[CornerIndex] * (CornerTangentZ[CornerIndex] | CornerTangentX[CornerIndex]);
				CornerTangentX[CornerIndex].Normalize();
				CornerTangentY[CornerIndex] -= CornerTangentZ[CornerIndex] * (CornerTangentZ[CornerIndex] | CornerTangentY[CornerIndex]);
				CornerTangentY[CornerIndex].Normalize();
			}

			// Copy back to the mesh.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				WedgeTangentX[WedgeOffset + CornerIndex] = CornerTangentX[CornerIndex];
				WedgeTangentY[WedgeOffset + CornerIndex] = CornerTangentY[CornerIndex];
				WedgeTangentZ[WedgeOffset + CornerIndex] = CornerTangentZ[CornerIndex];
			}
		}

		assert(WedgeTangentX.size() == NumWedges);
		assert(WedgeTangentY.size() == NumWedges);
		assert(WedgeTangentZ.size() == NumWedges);
	}

	void Skeletal_ComputeTangents_MikkTSpace(
		IMeshBuildData* BuildData,
		const FOverlappingCorners& OverlappingCorners
	)
	{
		bool bBlendOverlappingNormals = true;
		bool bIgnoreDegenerateTriangles = BuildData->BuildOptions.bRemoveDegenerateTriangles;

		// Compute per-triangle tangents.
		std::vector<FVector> TriangleTangentX;
		std::vector<FVector> TriangleTangentY;
		std::vector<FVector> TriangleTangentZ;

		Skeletal_ComputeTriangleTangents(
			TriangleTangentX,
			TriangleTangentY,
			TriangleTangentZ,
			BuildData,
			bIgnoreDegenerateTriangles ? SMALL_NUMBER : 0.0f
		);

		std::vector<FVector>& WedgeTangentX = BuildData->GetTangentArray(0);
		std::vector<FVector>& WedgeTangentY = BuildData->GetTangentArray(1);
		std::vector<FVector>& WedgeTangentZ = BuildData->GetTangentArray(2);

		// Declare these out here to avoid reallocations.
		std::vector<FFanFace> RelevantFacesForCorner[3];
		std::vector<int32> AdjacentFaces;

		int32 NumFaces = BuildData->GetNumFaces();
		int32 NumWedges = BuildData->GetNumWedges();
		assert(NumFaces * 3 == NumWedges);

		bool bWedgeTSpace = false;

		if (WedgeTangentX.size() > 0 && WedgeTangentY.size() > 0)
		{
			bWedgeTSpace = true;
			for (uint32 WedgeIdx = 0; WedgeIdx < WedgeTangentX.size()
				&& WedgeIdx < WedgeTangentY.size(); ++WedgeIdx)
			{
				bWedgeTSpace = bWedgeTSpace && (!WedgeTangentX[WedgeIdx].IsNearlyZero()) && (!WedgeTangentY[WedgeIdx].IsNearlyZero());
			}
		}

		// Allocate storage for tangents if none were provided, and calculate normals for MikkTSpace.
		if (WedgeTangentZ.size() != NumWedges)
		{
			// normals are not included, so we should calculate them
			WedgeTangentZ.clear();
			WedgeTangentZ.resize(NumWedges);
		}

		// we need to calculate normals for MikkTSpace

		for (int32 FaceIndex = 0; FaceIndex < NumFaces; FaceIndex++)
		{
			int32 WedgeOffset = FaceIndex * 3;
			FVector CornerPositions[3];
			FVector CornerNormal[3];

			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				CornerNormal[CornerIndex] = FVector::ZeroVector;
				CornerPositions[CornerIndex] = BuildData->GetVertexPosition(FaceIndex, CornerIndex);
				RelevantFacesForCorner[CornerIndex].clear();
			}

			// Don't process degenerate triangles.
			if (PointsEqual(CornerPositions[0], CornerPositions[1], BuildData->BuildOptions.OverlappingThresholds)
				|| PointsEqual(CornerPositions[0], CornerPositions[2], BuildData->BuildOptions.OverlappingThresholds)
				|| PointsEqual(CornerPositions[1], CornerPositions[2], BuildData->BuildOptions.OverlappingThresholds))
			{
				continue;
			}

			// No need to process triangles if tangents already exist.
			bool bCornerHasNormal[3] = { 0 };
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				bCornerHasNormal[CornerIndex] = !WedgeTangentZ[WedgeOffset + CornerIndex].IsZero();
			}
			if (bCornerHasNormal[0] && bCornerHasNormal[1] && bCornerHasNormal[2])
			{
				continue;
			}

			// Start building a list of faces adjacent to this face.
			AdjacentFaces.clear();
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				int32 ThisCornerIndex = WedgeOffset + CornerIndex;
				const std::vector<int32>& DupVerts = OverlappingCorners.FindIfOverlapping(ThisCornerIndex);
				if (DupVerts.size() == 0)
				{
					AddUnique(AdjacentFaces,ThisCornerIndex / 3); // I am a "dup" of myself
				}
				for (uint32 k = 0; k < DupVerts.size(); k++)
				{
					AddUnique(AdjacentFaces, DupVerts[k] / 3);
				}
			}

			// We need to sort these here because the criteria for point equality is
			// exact, so we must ensure the exact same order for all dups.
			//AdjacentFaces.Sort();
			std::sort(AdjacentFaces.begin(), AdjacentFaces.end());
			// Process adjacent faces
			for (uint32 AdjacentFaceIndex = 0; AdjacentFaceIndex < AdjacentFaces.size(); AdjacentFaceIndex++)
			{
				int32 OtherFaceIndex = AdjacentFaces[AdjacentFaceIndex];
				for (int32 OurCornerIndex = 0; OurCornerIndex < 3; OurCornerIndex++)
				{
					if (bCornerHasNormal[OurCornerIndex])
						continue;

					FFanFace NewFanFace;
					int32 CommonIndexCount = 0;

					// Check for vertices in common.
					if (FaceIndex == OtherFaceIndex)
					{
						CommonIndexCount = 3;
						NewFanFace.LinkedVertexIndex = OurCornerIndex;
					}
					else
					{
						// Check matching vertices against main vertex .
						for (int32 OtherCornerIndex = 0; OtherCornerIndex < 3; OtherCornerIndex++)
						{
							if (PointsEqual(
								CornerPositions[OurCornerIndex],
								BuildData->GetVertexPosition(OtherFaceIndex, OtherCornerIndex),
								BuildData->BuildOptions.OverlappingThresholds
							))
							{
								CommonIndexCount++;
								NewFanFace.LinkedVertexIndex = OtherCornerIndex;
							}
						}
					}

					// Add if connected by at least one point. Smoothing matches are considered later.
					if (CommonIndexCount > 0)
					{
						NewFanFace.FaceIndex = OtherFaceIndex;
						NewFanFace.bFilled = (OtherFaceIndex == FaceIndex); // Starter face for smoothing floodfill.
						NewFanFace.bBlendTangents = NewFanFace.bFilled;
						NewFanFace.bBlendNormals = NewFanFace.bFilled;
						RelevantFacesForCorner[OurCornerIndex].push_back(NewFanFace);
					}
				}
			}

			// Find true relevance of faces for a vertex normal by traversing
			// smoothing-group-compatible connected triangle fans around common vertices.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				if (bCornerHasNormal[CornerIndex])
					continue;

				int32 NewConnections;
				do
				{
					NewConnections = 0;
					for (uint32 OtherFaceIdx = 0; OtherFaceIdx < RelevantFacesForCorner[CornerIndex].size(); OtherFaceIdx++)
					{
						FFanFace& OtherFace = RelevantFacesForCorner[CornerIndex][OtherFaceIdx];
						// The vertex' own face is initially the only face with bFilled == true.
						if (OtherFace.bFilled)
						{
							for (uint32 NextFaceIndex = 0; NextFaceIndex < RelevantFacesForCorner[CornerIndex].size(); NextFaceIndex++)
							{
								FFanFace& NextFace = RelevantFacesForCorner[CornerIndex][NextFaceIndex];
								if (!NextFace.bFilled) // && !NextFace.bBlendTangents)
								{
									if ((NextFaceIndex != OtherFaceIdx)
										&& (BuildData->GetFaceSmoothingGroups(NextFace.FaceIndex) & BuildData->GetFaceSmoothingGroups(OtherFace.FaceIndex)))
									{
										int32 CommonVertices = 0;
										int32 CommonNormalVertices = 0;
										for (int32 OtherCornerIndex = 0; OtherCornerIndex < 3; OtherCornerIndex++)
										{
											for (int32 NextCornerIndex = 0; NextCornerIndex < 3; NextCornerIndex++)
											{
												int32 NextVertexIndex = BuildData->GetVertexIndex(NextFace.FaceIndex, NextCornerIndex);
												int32 OtherVertexIndex = BuildData->GetVertexIndex(OtherFace.FaceIndex, OtherCornerIndex);
												if (PointsEqual(
													BuildData->GetVertexPosition(NextFace.FaceIndex, NextCornerIndex),
													BuildData->GetVertexPosition(OtherFace.FaceIndex, OtherCornerIndex),
													BuildData->BuildOptions.OverlappingThresholds))
												{
													CommonVertices++;
													if (bBlendOverlappingNormals
														|| NextVertexIndex == OtherVertexIndex)
													{
														CommonNormalVertices++;
													}
												}
											}
										}
										// Flood fill faces with more than one common vertices which must be touching edges.
										if (CommonVertices > 1)
										{
											NextFace.bFilled = true;
											NextFace.bBlendNormals = (CommonNormalVertices > 1);
											NewConnections++;
										}
									}
								}
							}
						}
					}
				} while (NewConnections > 0);
			}

			// Vertex normal construction.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				if (bCornerHasNormal[CornerIndex])
				{
					CornerNormal[CornerIndex] = WedgeTangentZ[WedgeOffset + CornerIndex];
				}
				else
				{
					for (uint32 RelevantFaceIdx = 0; RelevantFaceIdx < RelevantFacesForCorner[CornerIndex].size(); RelevantFaceIdx++)
					{
						FFanFace const& RelevantFace = RelevantFacesForCorner[CornerIndex][RelevantFaceIdx];
						if (RelevantFace.bFilled)
						{
							int32 OtherFaceIndex = RelevantFace.FaceIndex;
							if (RelevantFace.bBlendNormals)
							{
								CornerNormal[CornerIndex] += TriangleTangentZ[OtherFaceIndex];
							}
						}
					}
					if (!WedgeTangentZ[WedgeOffset + CornerIndex].IsZero())
					{
						CornerNormal[CornerIndex] = WedgeTangentZ[WedgeOffset + CornerIndex];
					}
				}
			}

			// Normalization.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				CornerNormal[CornerIndex].Normalize();
			}

			// Copy back to the mesh.
			for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			{
				WedgeTangentZ[WedgeOffset + CornerIndex] = CornerNormal[CornerIndex];
			}
		}

		if (WedgeTangentX.size() != NumWedges)
		{
			WedgeTangentX.clear();
			WedgeTangentX.resize(NumWedges);
		}
		if (WedgeTangentY.size() != NumWedges)
		{
			WedgeTangentY.clear();
			WedgeTangentY.resize(NumWedges);
		}

		//if (!bWedgeTSpace)
		{
			// we can use mikktspace to calculate the tangents		
			SMikkTSpaceContext MikkTContext;
			MikkTContext.m_pInterface = BuildData->GetMikkTInterface();
			MikkTContext.m_pUserData = BuildData->GetMikkTUserData();
			//MikkTContext.m_bIgnoreDegenerates = bIgnoreDegenerateTriangles;

			genTangSpaceDefault(&MikkTContext);
		}

		assert(WedgeTangentX.size() == NumWedges);
		assert(WedgeTangentY.size() == NumWedges);
		assert(WedgeTangentZ.size() == NumWedges);
	}

	bool PrepareSourceMesh(IMeshBuildData* BuildData)
	{
		assert(Stage == EStage::Uninit);

		FOverlappingCorners* pOverlappingCorners = new FOverlappingCorners;

		LODOverlappingCorners.push_back(pOverlappingCorners);
		FOverlappingCorners& OverlappingCorners = *pOverlappingCorners;

		float ComparisonThreshold = THRESH_POINTS_ARE_SAME;//GetComparisonThreshold(LODBuildSettings[LODIndex]);
		int32 NumWedges = BuildData->GetNumWedges();

		// Find overlapping corners to accelerate adjacency.
		Skeletal_FindOverlappingCorners(OverlappingCorners, BuildData, ComparisonThreshold);

		// Figure out if we should recompute normals and tangents.
		bool bRecomputeNormals = BuildData->BuildOptions.bComputeNormals;
		bool bRecomputeTangents = BuildData->BuildOptions.bComputeTangents;

		// Dump normals and tangents if we are recomputing them.
		if (bRecomputeTangents)
		{
			std::vector<FVector>& TangentX = BuildData->GetTangentArray(0);
			std::vector<FVector>& TangentY = BuildData->GetTangentArray(1);

			TangentX.clear();
			TangentX.resize(NumWedges);
			TangentY.clear();
			TangentY.resize(NumWedges);
		}
		if (bRecomputeNormals)
		{
			std::vector<FVector>& TangentZ = BuildData->GetTangentArray(2);
			TangentZ.clear();
			TangentZ.resize(NumWedges);
		}

		// Compute any missing tangents. MikkTSpace should be use only when the user want to recompute the normals or tangents otherwise should always fallback on builtin
		if (BuildData->BuildOptions.bUseMikkTSpace && (BuildData->BuildOptions.bComputeNormals || BuildData->BuildOptions.bComputeTangents))
		{
			Skeletal_ComputeTangents_MikkTSpace(BuildData, OverlappingCorners);
		}
		else
		{
			Skeletal_ComputeTangents(BuildData, OverlappingCorners);
		}

		// At this point the mesh will have valid tangents.
		BuildData->ValidateTangentArraySize();
		assert(LODOverlappingCorners.size() == 1);

		Stage = EStage::Prepared;
		return true;
	}

	bool GenerateSkeletalRenderMesh(IMeshBuildData* InBuildData)
	{
		assert(Stage == EStage::Prepared);

		SkeletalMeshBuildData& BuildData = *(SkeletalMeshBuildData*)InBuildData;

		// Find wedge influences.
		std::vector<int32>	WedgeInfluenceIndices;
		std::map<uint32, uint32> VertexIndexToInfluenceIndexMap;

		for (uint32 LookIdx = 0; LookIdx < (uint32)BuildData.Influences.size(); LookIdx++)
		{
			// Order matters do not allow the map to overwrite an existing value.
			if (VertexIndexToInfluenceIndexMap.find(BuildData.Influences[LookIdx].VertIndex) == VertexIndexToInfluenceIndexMap.end())
			{
				VertexIndexToInfluenceIndexMap.insert(std::make_pair(BuildData.Influences[LookIdx].VertIndex, LookIdx) );
			}
		}

		for (uint32 WedgeIndex = 0; WedgeIndex < BuildData.Wedges.size(); WedgeIndex++)
		{
			auto it = VertexIndexToInfluenceIndexMap.find(BuildData.Wedges[WedgeIndex].iVertex);
			if (it != VertexIndexToInfluenceIndexMap.end())
			{
				WedgeInfluenceIndices.push_back(it->second);
			}
			else
			{
				// we have missing influence vert,  we weight to root
				WedgeInfluenceIndices.push_back(0);

				// add warning message
				if (BuildData.OutWarningMessages)
				{
					char buffer[256];
					sprintf_s(buffer, sizeof(buffer), "Missing influence on vert %d. Weighting it to root.", BuildData.Wedges[WedgeIndex].iVertex);
					BuildData.OutWarningMessages->push_back(std::string(buffer));
					if (BuildData.OutWarningNames)
					{
						BuildData.OutWarningNames->push_back(std::string("FFbxErrors::SkeletalMesh_VertMissingInfluences"));
					}
				}
			}
		}

		assert(BuildData.Wedges.size() == WedgeInfluenceIndices.size());

		std::vector<FSkeletalMeshVertIndexAndZ> VertIndexAndZ;
		std::vector<FSoftSkinBuildVertex> RawVertices;

		VertIndexAndZ.clear();
		RawVertices.reserve(BuildData.Points.size());

		for (uint32 FaceIndex = 0; FaceIndex < BuildData.Faces.size(); FaceIndex++)
		{
			const FMeshFace& Face = BuildData.Faces[FaceIndex];

			for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
			{
				FSoftSkinBuildVertex Vertex;
				const uint32 WedgeIndex = BuildData.GetWedgeIndex(FaceIndex, VertexIndex);
				const FMeshWedge& Wedge = BuildData.Wedges[WedgeIndex];

				Vertex.Position = BuildData.GetVertexPosition(FaceIndex, VertexIndex);

				FVector TangentX, TangentY, TangentZ;
				TangentX = BuildData.TangentX[WedgeIndex].GetSafeNormal();
				TangentY = BuildData.TangentY[WedgeIndex].GetSafeNormal();
				TangentZ = BuildData.TangentZ[WedgeIndex].GetSafeNormal();

				TangentX.Normalize();
				TangentY.Normalize();
				TangentZ.Normalize();

				Vertex.TangentX = TangentX;
				Vertex.TangentY = TangentY;
				Vertex.TangentZ = TangentZ;

				memcpy(Vertex.UVs, Wedge.UVs, sizeof(Vector2)*4/*MAX_TEXCOORDS*/);
				Vertex.Color = Wedge.Color;

				{
					// Count the influences.
					int32 InfIdx = WedgeInfluenceIndices[Face.iWedge[VertexIndex]];
					int32 LookIdx = InfIdx;

					uint32 InfluenceCount = 0;
					while ((int32)BuildData.Influences.size() > LookIdx && (BuildData.Influences[LookIdx].VertIndex == Wedge.iVertex))
					{
						InfluenceCount++;
						LookIdx++;
					}
					InfluenceCount = FMath::Min<uint32>(InfluenceCount, MAX_TOTAL_INFLUENCES);

					// Setup the vertex influences.
					Vertex.InfluenceBones[0] = 0;
					Vertex.InfluenceWeights[0] = 255;
					for (uint32 i = 1; i < MAX_TOTAL_INFLUENCES; i++)
					{
						Vertex.InfluenceBones[i] = 0;
						Vertex.InfluenceWeights[i] = 0;
					}

					uint32	TotalInfluenceWeight = 0;
					for (uint32 i = 0; i < InfluenceCount; i++)
					{
						FBoneIndexType BoneIndex = (FBoneIndexType)BuildData.Influences[InfIdx + i].BoneIndex;
						if (BoneIndex >= BuildData.RefSkeleton.GetRawBoneNum())
							continue;

						Vertex.InfluenceBones[i] = BoneIndex;
						Vertex.InfluenceWeights[i] = (uint8)(BuildData.Influences[InfIdx + i].Weight * 255.0f);
						TotalInfluenceWeight += Vertex.InfluenceWeights[i];
					}
					Vertex.InfluenceWeights[0] += 255 - TotalInfluenceWeight;
				}

				// Add the vertex as well as its original index in the points array
				Vertex.PointWedgeIdx = Wedge.iVertex;

				RawVertices.push_back(Vertex);
				int32 RawIndex = RawVertices.size() - 1;

				// Add an efficient way to find dupes of this vertex later for fast combining of vertices
				FSkeletalMeshVertIndexAndZ IAndZ;
				IAndZ.Index = RawIndex;
				IAndZ.Z = Vertex.Position.Z;

				VertIndexAndZ.push_back(IAndZ);
			}
		}

		// Generate chunks and their vertices and indices
		SkeletalMeshTools::BuildSkeletalMeshChunks(BuildData.Faces, RawVertices, VertIndexAndZ, BuildData.BuildOptions.OverlappingThresholds, BuildData.Chunks, BuildData.bTooManyVerts);

		// Chunk vertices to satisfy the requested limit.
		const uint32 MaxGPUSkinBones = 256 /*FGPUBaseSkinVertexFactory::GetMaxGPUSkinBones()*/;
		//assert(MaxGPUSkinBones <= FGPUBaseSkinVertexFactory::GHardwareMaxGPUSkinBones);
		SkeletalMeshTools::ChunkSkinnedVertices(BuildData.Chunks, MaxGPUSkinBones);

		Stage = EStage::GenerateRendering;
		return true;
	}
private:
	enum class EStage
	{
		Uninit,
		Prepared,
		GenerateRendering,
	};

	std::vector<FOverlappingCorners*> LODOverlappingCorners;
	EStage Stage;
};

void FOverlappingCorners::Init(int32 NumIndices)
{
	Arrays.clear();
	Sets.clear();
	bFinishedAdding = false;

	IndexBelongsTo.clear();
	IndexBelongsTo.resize(NumIndices);
	memset(IndexBelongsTo.data(), 0xFF, NumIndices * sizeof(int32));
}

void FOverlappingCorners::Add(int32 Key, int32 Value)
{
	assert(Key != Value);
	assert(bFinishedAdding == false);

	int32 ContainerIndex = IndexBelongsTo[Key];
	if (ContainerIndex == -1)
	{
		ContainerIndex = Arrays.size();
		Arrays.push_back(std::vector<int32>());
		std::vector<int32>& Container = Arrays[Arrays.size()-1];
		Container.reserve(6);
		Container.push_back(Key);
		Container.push_back(Value);
		IndexBelongsTo[Key] = ContainerIndex;
		IndexBelongsTo[Value] = ContainerIndex;
	}
	else
	{
		IndexBelongsTo[Value] = ContainerIndex;

		std::vector<int32>& ArrayContainer = Arrays[ContainerIndex];
		if (ArrayContainer.size() == 1)
		{
			// Container is a set
			Sets[ArrayContainer.back()].insert(Value);
		}
		else
		{
			// Container is an array
			if (std::find(ArrayContainer.begin(), ArrayContainer.end(), Value) == ArrayContainer.end())
			{
				ArrayContainer.push_back(Value);
			}

			// Change container into set when one vertex is shared by large number of triangles
			if (ArrayContainer.size() > 12)
			{
				uint32 SetIndex = Sets.size();
				Sets.push_back(std::set<int32>());
				std::set<int32>& Set = Sets[Sets.size() - 1];
				Set.insert(ArrayContainer.begin(),ArrayContainer.end());

				// Having one element means we are using a set
				// An array will never have just 1 element normally because we add them as pairs
				ArrayContainer.clear();
				ArrayContainer.push_back(SetIndex);
			}
		}
	}
}

void FOverlappingCorners::FinishAdding()
{
	assert(bFinishedAdding == false);

	for (std::vector<int32>& Array : Arrays)
	{
		// Turn sets back into arrays for easier iteration code
		// Also reduces peak memory later in the import process
		if (Array.size() == 1)
		{
			std::set<int32>& Set = Sets[Array.back()];
			Array.clear();
			for (int32 i : Set)
			{
				Array.push_back(i);
			}
		}

		// Sort arrays now to avoid sort multiple times
		//Array.Sort();
		std::sort(Array.begin(), Array.end());
	}

	Sets.clear();

	bFinishedAdding = true;
}


bool SkeletalMeshTools::AreSkelMeshVerticesEqual(const FSoftSkinBuildVertex& V1, const FSoftSkinBuildVertex& V2, const FOverlappingThresholds& OverlappingThresholds)
{
	if (!PointsEqual(V1.Position, V2.Position, OverlappingThresholds))
	{
		return false;
	}

	for (int32 UVIdx = 0; UVIdx < 4/*MAX_TEXCOORDS*/; ++UVIdx)
	{
		if (!UVsEqual(V1.UVs[UVIdx], V2.UVs[UVIdx], OverlappingThresholds))
		{
			return false;
		}
	}

	if (!NormalsEqual(V1.TangentX, V2.TangentX, OverlappingThresholds))
	{
		return false;
	}

	if (!NormalsEqual(V1.TangentY, V2.TangentY, OverlappingThresholds))
	{
		return false;
	}

	if (!NormalsEqual(V1.TangentZ, V2.TangentZ, OverlappingThresholds))
	{
		return false;
	}

	bool	InfluencesMatch = 1;
	for (uint32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
	{
		if (V1.InfluenceBones[InfluenceIndex] != V2.InfluenceBones[InfluenceIndex] ||
			V1.InfluenceWeights[InfluenceIndex] != V2.InfluenceWeights[InfluenceIndex])
		{
			InfluencesMatch = 0;
			break;
		}
	}

	if (V1.Color != V2.Color)
	{
		return false;
	}

	if (!InfluencesMatch)
	{
		return false;
	}

	return true;
}

// uint32 FOverlappingCorners::GetAllocatedSize(void) const
// {
// 	uint32 BaseMemoryAllocated = IndexBelongsTo.GetAllocatedSize() + Arrays.GetAllocatedSize() + Sets.GetAllocatedSize();
// 
// 	uint32 ArraysMemory = 0;
// 	for (const TArray<int32>& ArrayIt : Arrays)
// 	{
// 		ArraysMemory += ArrayIt.GetAllocatedSize();
// 	}
// 
// 	uint32 SetsMemory = 0;
// 	for (const TSet<int32>& SetsIt : Sets)
// 	{
// 		SetsMemory += SetsIt.GetAllocatedSize();
// 	}
// 
// 	return BaseMemoryAllocated + ArraysMemory + SetsMemory;
// }

void SkeletalMeshTools::BuildSkeletalMeshChunks(const std::vector<FMeshFace>& Faces, const std::vector<FSoftSkinBuildVertex>& RawVertices, std::vector<FSkeletalMeshVertIndexAndZ>& RawVertIndexAndZ, const FOverlappingThresholds &OverlappingThresholds, std::vector<FSkinnedMeshChunk*>& OutChunks, bool& bOutTooManyVerts)
{
	std::vector<int32> DupVerts;

	std::multimap<int32, int32> RawVerts2Dupes;
	{

		// Sorting function for vertex Z/index pairs
		struct FCompareFSkeletalMeshVertIndexAndZ
		{
			FORCEINLINE bool operator()(const FSkeletalMeshVertIndexAndZ& A, const FSkeletalMeshVertIndexAndZ& B) const
			{
				return A.Z < B.Z;
			}
		};

		// Sort the vertices by z value
		//RawVertIndexAndZ.Sort(FCompareFSkeletalMeshVertIndexAndZ());
		std::sort(RawVertIndexAndZ.begin(), RawVertIndexAndZ.end(), FCompareFSkeletalMeshVertIndexAndZ());

		// Search for duplicates, quickly!
		for (uint32 i = 0; i < RawVertIndexAndZ.size(); i++)
		{
			// only need to search forward, since we add pairs both ways
			for (uint32 j = i + 1; j < RawVertIndexAndZ.size(); j++)
			{
				if (FMath::Abs(RawVertIndexAndZ[j].Z - RawVertIndexAndZ[i].Z) > OverlappingThresholds.ThresholdPosition)
				{
					// our list is sorted, so there can't be any more dupes
					break;
				}

				// check to see if the points are really overlapping
				if (PointsEqual(
					RawVertices[RawVertIndexAndZ[i].Index].Position,
					RawVertices[RawVertIndexAndZ[j].Index].Position, OverlappingThresholds))
				{
					RawVerts2Dupes.insert(std::make_pair(RawVertIndexAndZ[i].Index, RawVertIndexAndZ[j].Index) );
					RawVerts2Dupes.insert(std::make_pair(RawVertIndexAndZ[j].Index, RawVertIndexAndZ[i].Index));
				}
			}
		}
	}

	std::map<FSkinnedMeshChunk*, std::map<int32, int32> > ChunkToFinalVerts;


	uint32 TriangleIndices[3];
	for (uint32 FaceIndex = 0; FaceIndex < Faces.size(); FaceIndex++)
	{
		const FMeshFace& Face = Faces[FaceIndex];

		// Find a chunk which matches this triangle.
		FSkinnedMeshChunk* Chunk = NULL;
		for (uint32 i = 0; i < OutChunks.size(); ++i)
		{
			if (OutChunks[i]->MaterialIndex == Face.MeshMaterialIndex)
			{
				Chunk = OutChunks[i];
				break;
			}
		}
		if (Chunk == NULL)
		{
			Chunk = new FSkinnedMeshChunk();
			Chunk->MaterialIndex = Face.MeshMaterialIndex;
			Chunk->OriginalSectionIndex = (int32)OutChunks.size();
			OutChunks.push_back(Chunk);
		}

		std::map<int32, int32>& FinalVerts = ChunkToFinalVerts[Chunk];

		for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
		{
			int32 WedgeIndex = FaceIndex * 3 + VertexIndex;
			const FSoftSkinBuildVertex& Vertex = RawVertices[WedgeIndex];

			int32 FinalVertIndex = -1;
			DupVerts.clear();
			//RawVerts2Dupes.MultiFind(WedgeIndex, DupVerts);
			auto range = RawVerts2Dupes.equal_range(WedgeIndex);
			for (auto i=range.first;i!=range.second;++i)
			{
				DupVerts.push_back(i->second);
			}
			//DupVerts.insert(DupVerts.begin(), range.first, range.second);
			std::sort(DupVerts.begin(),DupVerts.end());


			for (uint32 k = 0; k < DupVerts.size(); k++)
			{
				if (DupVerts[k] >= WedgeIndex)
				{
					// the verts beyond me haven't been placed yet, so these duplicates are not relevant
					break;
				}

				//int32 *Location = FinalVerts.Find(DupVerts[k]);
				auto it = FinalVerts.find(DupVerts[k]);
				if (it != FinalVerts.end())
				{
					if (SkeletalMeshTools::AreSkelMeshVerticesEqual(Vertex, Chunk->Vertices[it->second], OverlappingThresholds))
					{
						FinalVertIndex = it->second;
						break;
					}
				}
			}
			if (FinalVertIndex == -1)
			{
				Chunk->Vertices.push_back(Vertex);
				FinalVertIndex = (int32)Chunk->Vertices.size() - 1;
				FinalVerts.insert(std::make_pair(WedgeIndex, FinalVertIndex));
			}

			// set the index entry for the newly added vertex
			// TArray internally has int32 for capacity, so no need to test for uint32 as it's larger than int32
			TriangleIndices[VertexIndex] = (uint32)FinalVertIndex;
		}

		if (TriangleIndices[0] != TriangleIndices[1] && TriangleIndices[0] != TriangleIndices[2] && TriangleIndices[1] != TriangleIndices[2])
		{
			for (uint32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
			{
				Chunk->Indices.push_back(TriangleIndices[VertexIndex]);
			}
		}
	}
}

void SkeletalMeshTools::ChunkSkinnedVertices(std::vector<FSkinnedMeshChunk*>& Chunks, int32 MaxBonesPerChunk)
{
	// Copy over the old chunks (this is just copying pointers).
	std::vector<FSkinnedMeshChunk*> SrcChunks;
	Exchange(Chunks, SrcChunks);

	// Sort the chunks by material index.
	struct FCompareSkinnedMeshChunk
	{
		FORCEINLINE bool operator()(const FSkinnedMeshChunk* A, const FSkinnedMeshChunk* B) const
		{
			return A->MaterialIndex < B->MaterialIndex;
		}
	};
	std::sort(SrcChunks.begin(), SrcChunks.end(), FCompareSkinnedMeshChunk());
	//SrcChunks.Sort(FCompareSkinnedMeshChunk());

	// Now split chunks to respect the desired bone limit.
	std::vector<std::vector<int32>* > IndexMaps;
	std::vector<FBoneIndexType> UniqueBones;
	for (uint32 SrcChunkIndex = 0; SrcChunkIndex < SrcChunks.size(); ++SrcChunkIndex)
	{
		FSkinnedMeshChunk* SrcChunk = SrcChunks[SrcChunkIndex];
		uint32 FirstChunkIndex = Chunks.size();

		for (uint32 i = 0; i < SrcChunk->Indices.size(); i += 3)
		{
			// Find all bones needed by this triangle.
			UniqueBones.clear();
			for (int32 Corner = 0; Corner < 3; Corner++)
			{
				int32 VertexIndex = SrcChunk->Indices[i + Corner];
				FSoftSkinBuildVertex& V = SrcChunk->Vertices[VertexIndex];
				for (int32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
				{
					if (V.InfluenceWeights[InfluenceIndex] > 0)
					{
						//UniqueBones.AddUnique(V.InfluenceBones[InfluenceIndex]);
						AddUnique(UniqueBones, V.InfluenceBones[InfluenceIndex]);
					}
				}
			}

			// Now find a chunk for them.
			FSkinnedMeshChunk* DestChunk = NULL;
			uint32 DestChunkIndex = FirstChunkIndex;
			for (; DestChunkIndex < Chunks.size(); ++DestChunkIndex)
			{
				std::vector<FBoneIndexType>& BoneMap = Chunks[DestChunkIndex]->BoneMap;
				int32 NumUniqueBones = 0;
				for (uint32 j = 0; j < UniqueBones.size(); ++j)
				{
					NumUniqueBones += (Contains(BoneMap,UniqueBones[j]) ? 0 : 1);
				}
				if (NumUniqueBones + (int32)BoneMap.size() <= MaxBonesPerChunk)
				{
					DestChunk = Chunks[DestChunkIndex];
					break;
				}
			}

			// If no chunk was found, create one!
			if (DestChunk == NULL)
			{
				DestChunk = new FSkinnedMeshChunk();
				Chunks.push_back(DestChunk);
				DestChunk->MaterialIndex = SrcChunk->MaterialIndex;
				DestChunk->OriginalSectionIndex = SrcChunk->OriginalSectionIndex;
				std::vector<int32>* pIndexMap = new std::vector<int32>();
				IndexMaps.push_back(pIndexMap);
				std::vector<int32>& IndexMap = *pIndexMap;
				IndexMap.resize(SrcChunk->Vertices.size());
				memset(IndexMap.data(), 0xff, sizeof(std::vector<int32>::value_type) * IndexMap.size());
			}
			std::vector<int32>& IndexMap = *IndexMaps[DestChunkIndex];

			// Add the unique bones to this chunk's bone map.
			for (uint32 j = 0; j < UniqueBones.size(); ++j)
			{
				AddUnique(DestChunk->BoneMap,UniqueBones[j]);
			}

			// For each vertex, add it to the chunk's arrays of vertices and indices.
			for (int32 Corner = 0; Corner < 3; Corner++)
			{
				int32 VertexIndex = SrcChunk->Indices[i + Corner];
				int32 DestIndex = IndexMap[VertexIndex];
				if (DestIndex == -1)
				{
					DestChunk->Vertices.push_back(SrcChunk->Vertices[VertexIndex]);
					DestIndex = (int32)DestChunk->Vertices.size() - 1;
					FSoftSkinBuildVertex& V = DestChunk->Vertices[DestIndex];
					for (int32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
					{
						if (V.InfluenceWeights[InfluenceIndex] > 0)
						{
							int32 MappedIndex = Find(DestChunk->BoneMap, V.InfluenceBones[InfluenceIndex]);
							assert(IsValidIndex(DestChunk->BoneMap,MappedIndex));
							V.InfluenceBones[InfluenceIndex] = MappedIndex;
						}
					}
					IndexMap[VertexIndex] = DestIndex;
				}
				DestChunk->Indices.push_back(DestIndex);
			}
		}

		// Source chunks are no longer needed.
		delete SrcChunks[SrcChunkIndex];
		SrcChunks[SrcChunkIndex] = NULL;
	}
}

bool FBXImporter::BuildSkeletalMesh(FSkeletalMeshLODModel& LODModel, const FReferenceSkeleton& RefSkeleton, const std::vector<FVertInfluence>& Influences, const std::vector<FMeshWedge>& Wedges, const std::vector<FMeshFace>& Faces, const std::vector<FVector>& Points, const std::vector<int32>& PointToOriginalMap, const MeshBuildOptions& BuildOptions /*= MeshBuildOptions()*/, std::vector<std::string> * OutWarningMessages /*= NULL*/, std::vector<std::string> * OutWarningNames /*= NULL*/)
{
	auto UpdateOverlappingVertices = [](FSkeletalMeshLODModel& InLODModel)
	{
		// clear first
		for (uint32 SectionIdx = 0; SectionIdx < InLODModel.Sections.size(); SectionIdx++)
		{
			FSkeletalMeshSection& CurSection = InLODModel.Sections[SectionIdx];
			CurSection.OverlappingVertices.clear();
		}

		for (uint32 SectionIdx = 0; SectionIdx < InLODModel.Sections.size(); SectionIdx++)
		{
			FSkeletalMeshSection& CurSection = InLODModel.Sections[SectionIdx];
			const uint32 NumSoftVertices = CurSection.SoftVertices.size();

			// Create a list of vertex Z/index pairs
			std::vector<FIndexAndZ> VertIndexAndZ;
			VertIndexAndZ.reserve(NumSoftVertices);
			for (uint32 VertIndex = 0; VertIndex < NumSoftVertices; ++VertIndex)
			{
				FSoftSkinVertex& SrcVert = CurSection.SoftVertices[VertIndex];
				//new(VertIndexAndZ)FIndexAndZ(VertIndex, SrcVert.Position);
				VertIndexAndZ.push_back(FIndexAndZ(VertIndex, SrcVert.Position));
			}
			//VertIndexAndZ.Sort(FCompareIndexAndZ());
			std::sort(VertIndexAndZ.begin(), VertIndexAndZ.end(), FCompareIndexAndZ());

			// Search for duplicates, quickly!
			for (uint32 i = 0; i < VertIndexAndZ.size(); ++i)
			{
				const uint32 SrcVertIndex = VertIndexAndZ[i].Index;
				const float Z = VertIndexAndZ[i].Z;
				FSoftSkinVertex& SrcVert = CurSection.SoftVertices[SrcVertIndex];

				// only need to search forward, since we add pairs both ways
				for (uint32 j = i + 1; j < VertIndexAndZ.size(); ++j)
				{
					if (FMath::Abs(VertIndexAndZ[j].Z - Z) > THRESH_POINTS_ARE_SAME)
						break; // can't be any more dups

					const uint32 IterVertIndex = VertIndexAndZ[j].Index;
					FSoftSkinVertex& IterVert = CurSection.SoftVertices[IterVertIndex];
					if (PointsEqual(SrcVert.Position, IterVert.Position))
					{
						// if so, we add to overlapping vert
						std::vector<int32>& SrcValueArray = CurSection.OverlappingVertices[SrcVertIndex];
						SrcValueArray.push_back(IterVertIndex);

						std::vector<int32>& IterValueArray = CurSection.OverlappingVertices[IterVertIndex];
						IterValueArray.push_back(SrcVertIndex);
					}
				}
			}
		}
	};


	SkeletalMeshBuildData BuildData(
		LODModel,
		RefSkeleton,
		Influences,
		Wedges,
		Faces,
		Points,
		PointToOriginalMap,
		BuildOptions,
		OutWarningMessages,
		OutWarningNames);

	FSkeletalMeshUtilityBuilder Builder;
	if (!Builder.PrepareSourceMesh(&BuildData))
	{
		return false;
	}

	if (!Builder.GenerateSkeletalRenderMesh(&BuildData))
	{
		return false;
	}

	// Build the skeletal model from chunks.
	BuildSkeletalModelFromChunks(BuildData.LODModel, BuildData.RefSkeleton, BuildData.Chunks, BuildData.PointToOriginalMap);
	UpdateOverlappingVertices(BuildData.LODModel);

	return true;
}

void FBXImporter::BuildSkeletalModelFromChunks(FSkeletalMeshLODModel& LODModel, const FReferenceSkeleton& RefSkeleton, std::vector<FSkinnedMeshChunk*>& Chunks, const std::vector<int32>& PointToOriginalMap)
{
	// Clear out any data currently held in the LOD model.
	LODModel.Sections.clear();
	LODModel.NumVertices = 0;
	LODModel.IndexBuffer.clear();

	// Setup the section and chunk arrays on the model.
	for (uint32 ChunkIndex = 0; ChunkIndex < Chunks.size(); ++ChunkIndex)
	{
		FSkinnedMeshChunk* SrcChunk = Chunks[ChunkIndex];

		LODModel.Sections.push_back(FSkeletalMeshSection());
		FSkeletalMeshSection& Section = LODModel.Sections.back();
		Section.MaterialIndex = SrcChunk->MaterialIndex;
		Exchange(Section.BoneMap, SrcChunk->BoneMap);

		// Update the active bone indices on the LOD model.
		for (uint32 BoneIndex = 0; BoneIndex < Section.BoneMap.size(); ++BoneIndex)
		{
			AddUnique(LODModel.ActiveBoneIndices, Section.BoneMap[BoneIndex]);
		}
	}

	// ensure parent exists with incoming active bone indices, and the result should be sorted	
	RefSkeleton.EnsureParentsExistAndSort(LODModel.ActiveBoneIndices);

	// Reset 'final vertex to import vertex' map info
	LODModel.MeshToImportVertexMap.clear();
	LODModel.MaxImportVertex = 0;

	// Keep track of index mapping to chunk vertex offsets
	std::vector< std::vector<uint32> > VertexIndexRemap;
	VertexIndexRemap.clear();
	// Pack the chunk vertices into a single vertex buffer.
	std::vector<uint32> RawPointIndices;
	LODModel.NumVertices = 0;

	int32 PrevMaterialIndex = -1;
	int32 CurrentChunkBaseVertexIndex = -1; 	// base vertex index for all chunks of the same material
	int32 CurrentChunkVertexCount = -1; 		// total vertex count for all chunks of the same material
	int32 CurrentVertexIndex = 0; 			// current vertex index added to the index buffer for all chunks of the same material

											// rearrange the vert order to minimize the data fetched by the GPU
	for (uint32 SectionIndex = 0; SectionIndex < LODModel.Sections.size(); SectionIndex++)
	{
		FSkinnedMeshChunk* SrcChunk = Chunks[SectionIndex];
		FSkeletalMeshSection& Section = LODModel.Sections[SectionIndex];
		std::vector<FSoftSkinBuildVertex>& ChunkVertices = SrcChunk->Vertices;
		std::vector<uint32>& ChunkIndices = SrcChunk->Indices;

		// Reorder the section index buffer for better vertex cache efficiency.
		//CacheOptimizeIndexBuffer(ChunkIndices);

		// Calculate the number of triangles in the section.  Note that CacheOptimize may change the number of triangles in the index buffer!
		Section.NumTriangles = ChunkIndices.size() / 3;
		std::vector<FSoftSkinBuildVertex> OriginalVertices;
		Exchange(ChunkVertices, OriginalVertices);
		ChunkVertices.resize(OriginalVertices.size());

		std::vector<int32> IndexCache;
		IndexCache.resize(ChunkVertices.size());
		memset(IndexCache.data(), -1, IndexCache.size() * sizeof(std::vector<int32>::value_type));
		int32 NextAvailableIndex = 0;
		// Go through the indices and assign them new values that are coherent where possible
		for (uint32 Index = 0; Index < ChunkIndices.size(); Index++)
		{
			const int32 OriginalIndex = ChunkIndices[Index];
			const int32 CachedIndex = IndexCache[OriginalIndex];

			if (CachedIndex == -1)
			{
				// No new index has been allocated for this existing index, assign a new one
				ChunkIndices[Index] = NextAvailableIndex;
				// Mark what this index has been assigned to
				IndexCache[OriginalIndex] = NextAvailableIndex;
				NextAvailableIndex++;
			}
			else
			{
				// Reuse an existing index assignment
				ChunkIndices[Index] = CachedIndex;
			}
			// Reorder the vertices based on the new index assignment
			ChunkVertices[ChunkIndices[Index]] = OriginalVertices[OriginalIndex];
		}
	}

	// Build the arrays of rigid and soft vertices on the model's chunks.
	for (uint32 SectionIndex = 0; SectionIndex < LODModel.Sections.size(); SectionIndex++)
	{
		FSkeletalMeshSection& Section = LODModel.Sections[SectionIndex];
		std::vector<FSoftSkinBuildVertex>& ChunkVertices = Chunks[SectionIndex]->Vertices;

		CurrentVertexIndex = 0;
		CurrentChunkVertexCount = 0;
		PrevMaterialIndex = Section.MaterialIndex;

		// Calculate the offset to this chunk's vertices in the vertex buffer.
		Section.BaseVertexIndex = CurrentChunkBaseVertexIndex = LODModel.NumVertices;

		// Update the size of the vertex buffer.
		LODModel.NumVertices += ChunkVertices.size();

		// Separate the section's vertices into rigid and soft vertices.
		VertexIndexRemap.push_back(std::vector<uint32>());
		std::vector<uint32>& ChunkVertexIndexRemap = VertexIndexRemap.back();
		ChunkVertexIndexRemap.resize(ChunkVertices.size());

		for (uint32 VertexIndex = 0; VertexIndex < ChunkVertices.size(); VertexIndex++)
		{
			const FSoftSkinBuildVertex& SoftVertex = ChunkVertices[VertexIndex];

			FSoftSkinVertex NewVertex;
			NewVertex.Position = SoftVertex.Position;
			NewVertex.TangentX = SoftVertex.TangentX;
			NewVertex.TangentY = SoftVertex.TangentY;
			NewVertex.TangentZ = SoftVertex.TangentZ;
			memcpy(NewVertex.UVs, SoftVertex.UVs, sizeof(Vector2)*4/*MAX_TEXCOORDS*/);
			NewVertex.Color = SoftVertex.Color;
			for (int32 i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
			{
				// it only adds to the bone map if it has weight on it
				// BoneMap contains only the bones that has influence with weight of >0.f
				// so here, just make sure it is included before setting the data
				if (IsValidIndex(Section.BoneMap,SoftVertex.InfluenceBones[i]))
				{
					NewVertex.InfluenceBones[i] = (uint8)SoftVertex.InfluenceBones[i];
					NewVertex.InfluenceWeights[i] = (uint8)SoftVertex.InfluenceWeights[i];
				}
			}
			Section.SoftVertices.push_back(NewVertex);
			ChunkVertexIndexRemap[VertexIndex] = (uint32)(Section.BaseVertexIndex + CurrentVertexIndex);
			CurrentVertexIndex++;
			// add the index to the original wedge point source of this vertex
			RawPointIndices.push_back(SoftVertex.PointWedgeIdx);
			// Also remember import index
			const int32 RawVertIndex = PointToOriginalMap[SoftVertex.PointWedgeIdx];
			LODModel.MeshToImportVertexMap.push_back(RawVertIndex);
			LODModel.MaxImportVertex = FMath::Max<int32>(LODModel.MaxImportVertex, RawVertIndex);
		}

		// update NumVertices
		Section.NumVertices = (int32)Section.SoftVertices.size();

		// update max bone influences
		Section.CalcMaxBoneInfluences();

		// Log info about the chunk.
		//X_LOG("Section %u: %u vertices, %u active bones", SectionIndex, Section.GetNumVertices(), Section.BoneMap.size())
	}

	// Copy raw point indices to LOD model.
// 	LODModel.RawPointIndices.RemoveBulkData();
// 	if (RawPointIndices.Num())
// 	{
// 		LODModel.RawPointIndices.Lock(LOCK_READ_WRITE);
// 		void* Dest = LODModel.RawPointIndices.Realloc(RawPointIndices.Num());
// 		memcpy(Dest, RawPointIndices.data(), LODModel.RawPointIndices.GetBulkDataSize());
// 		LODModel.RawPointIndices.Unlock();
// 	}

	// Finish building the sections.
	for (uint32 SectionIndex = 0; SectionIndex < LODModel.Sections.size(); SectionIndex++)
	{
		FSkeletalMeshSection& Section = LODModel.Sections[SectionIndex];

		const std::vector<uint32>& SectionIndices = Chunks[SectionIndex]->Indices;

		Section.BaseIndex = LODModel.IndexBuffer.size();
		const uint32 NumIndices = SectionIndices.size();
		const std::vector<uint32>& SectionVertexIndexRemap = VertexIndexRemap[SectionIndex];
		for (uint32 Index = 0; Index < NumIndices; Index++)
		{
			uint32 VertexIndex = SectionVertexIndexRemap[SectionIndices[Index]];
			LODModel.IndexBuffer.push_back(VertexIndex);
		}
	}

	// Free the skinned mesh chunks which are no longer needed.
	for (uint32 i = 0; i < Chunks.size(); ++i)
	{
		delete Chunks[i];
		Chunks[i] = NULL;
	}
	Chunks.clear();

	// Compute the required bones for this model.
	USkeletalMesh::CalculateRequiredBones(LODModel, RefSkeleton, NULL);
}

bool FBXImporter::BuildStaticMesh(FStaticMeshRenderData& OutRenderData, UStaticMesh* Mesh/*, const FStaticMeshLODGroup& LODGroup */)
{

	OutRenderData.AllocateLODResources(1);

	MeshDescription MD2;
	Mesh->GetRenderMeshDescription(Mesh->GetMeshDescription(), MD2);

	std::vector<std::vector<uint32> > OutPerSectionIndices;
	OutPerSectionIndices.resize(Mesh->GetMeshDescription().PolygonGroups().Num());
	std::vector<StaticMeshBuildVertex> StaticMeshBuildVertices;
	FStaticMeshLODResources& LODResource = *OutRenderData.LODResources[0];
	std::vector<int32> RemapVerts;
	BuildVertexBuffer(MD2, LODResource, OutPerSectionIndices, StaticMeshBuildVertices,Mesh->GetOverlappingCorners(), THRESH_POINTS_ARE_SAME, RemapVerts);

	std::vector<uint32> CombinedIndices;
	for (uint32 i = 0; i < LODResource.Sections.size(); ++i)
	{
		FStaticMeshSection& Section = LODResource.Sections[i];
		std::vector<uint32> const& SectionIndices = OutPerSectionIndices[i];
		Section.FirstIndex = 0;
		Section.NumTriangles = 0;
		Section.MinVertexIndex = 0;
		Section.MaxVertexIndex = 0;

		if (SectionIndices.size())
		{
			Section.FirstIndex = CombinedIndices.size();
			Section.NumTriangles = SectionIndices.size() / 3;

			CombinedIndices.resize(CombinedIndices.size() + SectionIndices.size(), 0);
			uint32* DestPtr = &CombinedIndices[Section.FirstIndex];
			uint32 const* SrcPtr = SectionIndices.data();

			Section.MinVertexIndex = *SrcPtr;
			Section.MaxVertexIndex = *DestPtr;
			Section.bCastShadow = true;
			for (uint32 Index = 0; Index < SectionIndices.size(); Index++)
			{
				uint32 VertIndex = *SrcPtr++;
				Section.MinVertexIndex = Section.MinVertexIndex > VertIndex ? VertIndex : Section.MinVertexIndex;
				Section.MaxVertexIndex = Section.MaxVertexIndex < VertIndex ? VertIndex : Section.MaxVertexIndex;
				*DestPtr++ = VertIndex;
			}
		}
	}
	LODResource.Indices = std::move(CombinedIndices);
	return true;
}
bool AreVerticesEqual(StaticMeshBuildVertex const& A, StaticMeshBuildVertex const& B, float ComparisonThreshold)
{
	if (!A.Position.Equals(B.Position, ComparisonThreshold)
		|| !NormalsEqual(A.TangentX, B.TangentX)
		|| !NormalsEqual(A.TangentY, B.TangentY)
		|| !NormalsEqual(A.TangentZ, B.TangentZ)
		/*|| A.Color != B.Color*/)
	{
		return false;
	}

	// UVs
	if (!UVsEqual(A.UVs, B.UVs))
	{
		return false;
	}
	if (!UVsEqual(A.LightMapCoordinate, B.LightMapCoordinate))
	{
		return false;
	}
	return true;
}
void FBXImporter::BuildVertexBuffer(const MeshDescription& MD2, FStaticMeshLODResources& StaticMeshLOD, std::vector<std::vector<uint32> >& OutPerSectionIndices, std::vector<StaticMeshBuildVertex>& StaticMeshBuildVertices, const std::multimap<int32, int32>& OverlappingCorners, float VertexComparisonThreshold, std::vector<int32>& RemapVerts)
{
	const TMeshElementArray<MeshVertex>& Vertices = MD2.Vertices();
	const TMeshElementArray<MeshVertexInstance>& VertexInstances = MD2.VertexInstances();
	const TMeshElementArray<MeshPolygonGroup>& PolygonGroupArray = MD2.PolygonGroups();
	const TMeshElementArray<MeshPolygon>& PolygonArray = MD2.Polygons();

	RemapVerts.resize(VertexInstances.Num());
	for (int32& RemapIndex : RemapVerts)
	{
		RemapIndex = INDEX_NONE;
	}
	std::vector<int32> DupVerts;

	const std::vector<std::string>& PolygonGroupImportedMaterialSlotNames = MD2.PolygonGroupAttributes().GetAttributes<std::string>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

	const std::vector<FVector>& VertexPositions = MD2.VertexAttributes().GetAttributes<FVector>(MeshAttribute::Vertex::Position);
	const std::vector<FVector>& VertexInstanceNormals = MD2.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Normal);
	const std::vector<FVector>& VertexInstanceTangents = MD2.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Tangent);
	const std::vector<float>& VertexInstanceBinormalSigns = MD2.VertexInstanceAttributes().GetAttributes<float>(MeshAttribute::VertexInstance::BinormalSign);
	const std::vector<Vector4>& VertexInstanceColors = MD2.VertexInstanceAttributes().GetAttributes<Vector4>(MeshAttribute::VertexInstance::Color);
	const std::vector<std::vector<Vector2>>& VertexInstanceUVs = MD2.VertexInstanceAttributes().GetAttributesSet<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);

	std::map<int, int> PolygonGroupToSectionIndex;
	for (const int PolgyonGroupID : MD2.PolygonGroups().GetElementIDs())
	{
		int& SectionIndex = PolygonGroupToSectionIndex[PolgyonGroupID];
		StaticMeshLOD.Sections.push_back(FStaticMeshSection());
		SectionIndex = (int)StaticMeshLOD.Sections.size() - 1;
		FStaticMeshSection& Section = StaticMeshLOD.Sections[SectionIndex];
		//if (Section.MaterialIndex == -1)
		{
			Section.MaterialIndex = PolgyonGroupID;
		}
	}

	int ReserveIndicesCount = 0;
	for (const int PolygonID : MD2.Polygons().GetElementIDs())
	{
		const std::vector<MeshTriangle>& PolygonTriangles = MD2.GetPolygonTriangles(PolygonID);
		ReserveIndicesCount += PolygonTriangles.size() * 3;
	}
	StaticMeshLOD.Indices.reserve(ReserveIndicesCount);

	for (const int PolygonID : MD2.Polygons().GetElementIDs())
	{
		const int PolygonGroupID = MD2.GetPolygonPolygonGroup(PolygonID);
		const int SectionIndex = PolygonGroupToSectionIndex[PolygonGroupID];
		std::vector<uint32>& SectionIndices = OutPerSectionIndices[SectionIndex];
		const std::vector<MeshTriangle>& PolygonTriangles = MD2.GetPolygonTriangles(PolygonID);
		uint32 MinIndex = 0;
		uint32 MaxIndex = 0xFFFFFFFF;
		for (int TriangleIndex = 0; TriangleIndex < (int)PolygonTriangles.size(); ++TriangleIndex)
		{
			const MeshTriangle& Triangle = PolygonTriangles[TriangleIndex];
			FVector CornerPositions[3];
			for (int TriVert = 0; TriVert < 3; ++TriVert)
			{
				const int VertexInstanceID = Triangle.GetVertexInstanceID(TriVert);
				const int VertexID = MD2.GetVertexInstanceVertex(VertexInstanceID);
				CornerPositions[TriVert] = VertexPositions[VertexID];
			}

			for (int TriVert = 0; TriVert < 3; ++TriVert)
			{
				const int VertexInstanceID = Triangle.GetVertexInstanceID(TriVert);
				const int VertexInstanceValue = VertexInstanceID;
				const FVector& VertexPosition = CornerPositions[TriVert];
				const FVector& VertexNormal = VertexInstanceNormals[VertexInstanceID];
				const FVector& VertexTangent = VertexInstanceTangents[VertexInstanceID];
				const float VertexInstanceBinormalSign = VertexInstanceBinormalSigns[VertexInstanceID];
				Vector2 UVs = VertexInstanceUVs[0][VertexInstanceID];
				Vector2 LightMapCoordinate = VertexInstanceUVs[1][VertexInstanceID];

				StaticMeshBuildVertex StaticMeshVertex;
				StaticMeshVertex.Position = VertexPosition;
				StaticMeshVertex.TangentX = VertexTangent;
				StaticMeshVertex.TangentY = (FVector::CrossProduct(VertexNormal, VertexTangent).GetSafeNormal() * VertexInstanceBinormalSign).GetSafeNormal();
				StaticMeshVertex.TangentZ = VertexNormal;
				StaticMeshVertex.UVs = UVs;
				StaticMeshVertex.LightMapCoordinate = LightMapCoordinate;

				DupVerts.clear();
				auto RangePair = OverlappingCorners.equal_range(VertexInstanceValue);
				for (auto i = RangePair.first; i != RangePair.second; ++i)
				{
					DupVerts.push_back(i->second);
				}
				std::sort(DupVerts.begin(),DupVerts.end());
				int32 Index = INDEX_NONE;
				for (uint32 k = 0; k < DupVerts.size(); k++)
				{
					if (DupVerts[k] >= VertexInstanceValue)
					{
						break;
					}
					int32 Location = IsValidIndex(RemapVerts,DupVerts[k]) ? RemapVerts[DupVerts[k]] : INDEX_NONE;
					if (Location != INDEX_NONE && AreVerticesEqual(StaticMeshVertex, StaticMeshBuildVertices[Location], VertexComparisonThreshold))
					{
						Index = Location;
						break;
					}
				}
				if (Index == INDEX_NONE)
				{
					Index = (int32)StaticMeshBuildVertices.size();
					StaticMeshBuildVertices.push_back(StaticMeshVertex);
				}
				RemapVerts[VertexInstanceValue] = Index;
				const uint32 RenderingVertexIndex = RemapVerts[VertexInstanceValue];
				//IndexBuffer.Add(RenderingVertexIndex);
				//OutWedgeMap[VertexInstanceValue] = RenderingVertexIndex;
				SectionIndices.push_back(RenderingVertexIndex);
			}
		}

	}
	StaticMeshLOD.VertexBuffers.PositionVertexBuffer.resize(StaticMeshBuildVertices.size());
	StaticMeshLOD.VertexBuffers.TangentsVertexBuffer.resize(StaticMeshBuildVertices.size()*2);
	StaticMeshLOD.VertexBuffers.TexCoordVertexBuffer.resize(StaticMeshBuildVertices.size()*2);
	for (size_t i = 0; i < StaticMeshBuildVertices.size(); ++i)
	{
		StaticMeshLOD.VertexBuffers.PositionVertexBuffer[i] = StaticMeshBuildVertices[i].Position;
		StaticMeshLOD.VertexBuffers.TangentsVertexBuffer[i * 2] = StaticMeshBuildVertices[i].TangentX;
		StaticMeshLOD.VertexBuffers.TangentsVertexBuffer[i * 2 + 1] =  Vector4( StaticMeshBuildVertices[i].TangentZ, GetBasisDeterminantSign(StaticMeshBuildVertices[i].TangentX, StaticMeshBuildVertices[i].TangentY, StaticMeshBuildVertices[i].TangentZ));
		StaticMeshLOD.VertexBuffers.TexCoordVertexBuffer[i * 2] = StaticMeshBuildVertices[i].UVs;
		StaticMeshLOD.VertexBuffers.TexCoordVertexBuffer[i * 2+ 1] = StaticMeshBuildVertices[i].LightMapCoordinate;
	}
}

