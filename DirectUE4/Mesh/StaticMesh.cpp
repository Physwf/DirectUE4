#include "StaticMesh.h"
#include "log.h"
#include "D3D11RHI.h"
#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>
#include "MeshDescriptionOperations.h"
#include "Scene.h"
#include "World.h"
#include "FBXImporter.h"

void MeshLODResources::InitResource()
{
	VertexBuffer = CreateVertexBuffer(false, sizeof(LocalVertex) * Vertices.size(), Vertices.data());
	PositionOnlyVertexBuffer = CreateVertexBuffer(false, sizeof(PositionOnlyLocalVertex) * Vertices.size(), PositionOnlyVertices.data());
	IndexBuffer = CreateIndexBuffer(Indices.data(), sizeof(unsigned int) * Indices.size());
}

void MeshLODResources::ReleaseResource()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
	}
	if (PositionOnlyVertexBuffer)
	{
		PositionOnlyVertexBuffer->Release();
	}
	if (IndexBuffer)
	{
		IndexBuffer->Release();
	}
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
		UniqueUVCount = (std::min)(UniqueUVCount,8);
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

void RegisterMeshAttributes(MeshDescription& MD)
{
	// Add basic vertex attributes
	MD.VertexAttributes().RegisterAttribute<Vector>(MeshAttribute::Vertex::Position, 1, Vector());
	MD.VertexAttributes().RegisterAttribute<float>(MeshAttribute::Vertex::CornerSharpness, 1, 0.0f);

	// Add basic vertex instance attributes
	MD.VertexInstanceAttributes().RegisterAttribute<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, 1, Vector2());
	MD.VertexInstanceAttributes().RegisterAttribute<Vector>(MeshAttribute::VertexInstance::Normal, 1, Vector());
	MD.VertexInstanceAttributes().RegisterAttribute<Vector>(MeshAttribute::VertexInstance::Tangent, 1, Vector());
	MD.VertexInstanceAttributes().RegisterAttribute<float>(MeshAttribute::VertexInstance::BinormalSign, 1, 0.0f);
	MD.VertexInstanceAttributes().RegisterAttribute<Vector4>(MeshAttribute::VertexInstance::Color, 1, Vector4(1.0f));

	// Add basic edge attributes
	MD.EdgeAttributes().RegisterAttribute<bool>(MeshAttribute::Edge::IsHard, 1, false);
	MD.EdgeAttributes().RegisterAttribute<bool>(MeshAttribute::Edge::IsUVSeam, 1, false);
	MD.EdgeAttributes().RegisterAttribute<float>(MeshAttribute::Edge::CreaseSharpness, 1, 0.0f);

	// Add basic polygon attributes
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Normal, 1, Vector());
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Tangent, 1, Vector());
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Binormal, 1, Vector());
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Center, 1, Vector());

	// Add basic polygon group attributes
	MD.PolygonGroupAttributes().RegisterAttribute<std::string>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName,1,std::string()); //The unique key to match the mesh material slot
}

StaticMesh::StaticMesh(const char* ResourcePath)
{
	ImportFromFBX(ResourcePath);
}

void StaticMesh::ImportFromFBX(const char* pFileName)
{
	FbxManager* lFbxManager = FbxManager::Create();

	FbxIOSettings* lIOSetting = FbxIOSettings::Create(lFbxManager, IOSROOT);
	lFbxManager->SetIOSettings(lIOSetting);

	FbxImporter* lImporter = FbxImporter::Create(lFbxManager, "MeshImporter");
	if (!lImporter->Initialize(pFileName,-1, lFbxManager->GetIOSettings()))
	{
		X_LOG("Call to FbxImporter::Initialize() failed.\n");
		X_LOG("Error returned: %s \n\n", lImporter->GetStatus().GetErrorString());
		return;
	}

	RegisterMeshAttributes(MD);

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
				FbxNodeAttribute* ConvertedNode = GeometryConverter->Triangulate(lMesh,true);

				if(ConvertedNode!=NULL && ConvertedNode->GetAttributeType() == FbxNodeAttribute::eMesh)
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

			bool bSmootingAvaliable = false;
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
					bSmootingAvaliable = true;
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
			TotalMatrix = ComputeTotalMatrix(lScene, lNode, true, false);
			TotalMatrixForNormal = TotalMatrix.Inverse();
			TotalMatrixForNormal = TotalMatrixForNormal.Transpose();

			std::vector<Vector>& VertexPositions =  MD.VertexAttributes().GetAttributes<Vector>(MeshAttribute::Vertex::Position);

			std::vector<Vector>& VertexInstanceNormals = MD.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Normal);
			std::vector<Vector>& VertexInstanceTangents = MD.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Tangent);
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
				Vector const VectorPositon = ConvertPos(FbxPosition);
				int VertexID = MD.CreateVertex();
				VertexPositions[VertexID] = VectorPositon;
			}

			int PolygonCount = lMesh->GetPolygonCount();
			int CurrentVertexInstanceIndex = 0;
			int SkippedVertexInstance = 0;

			lMesh->BeginGetMeshEdgeIndexForPolygon();
			for (auto PolygonIndex = 0; PolygonIndex < PolygonCount; ++PolygonIndex)
			{
				int PolygonVertexCount = lMesh->GetPolygonSize(PolygonIndex);
				std::vector<Vector> P;
				P.resize(PolygonVertexCount, Vector());
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
					const Vector VertexPostion = VertexPositions[VertexID];
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
						Vector TangentZ = ConvertDir(TempValue);
						VertexInstanceNormals[AddedVertexInstanceId] = TangentZ;

						if (bHasNTBInfomation)
						{
							int TangentMapIndex = (TangentMappingMode == FbxLayerElement::eByControlPoint) ? ControlPointIndex : RealFbxVertexIndex;
							int TangentValueIndex = (TangentReferenceMode == FbxLayerElement::eDirect) ? TangentMapIndex : LayerElementTangent->GetIndexArray().GetAt(TangentMapIndex);

							TempValue = LayerElementTangent->GetDirectArray().GetAt(TangentValueIndex);
							TempValue = TotalMatrixForNormal.MultT(TempValue);
							Vector TangentX = ConvertDir(TempValue);
							VertexInstanceTangents[AddedVertexInstanceId] = TangentX;

							int BinormalMapIndex = (BinormalMappingMode == FbxLayerElement::eByControlPoint) ? ControlPointIndex : RealFbxVertexIndex;
							int BinormalValueIndex = (BinormalReferenceMode == FbxLayerElement::eDirect) ? BinormalMapIndex : LayerElementBinormal->GetIndexArray().GetAt(BinormalMapIndex);

							TempValue = LayerElementBinormal->GetDirectArray().GetAt(BinormalValueIndex);
							TempValue = TotalMatrixForNormal.MultT(TempValue);
							Vector TangentY = -ConvertDir(TempValue);
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

						//int EdgeIndex = -1;


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

	Build();
}

void StaticMesh::GeneratePlane(float InWidth, float InHeight, int InNumSectionW, int InNumSectionH)
{
	LODResource.Vertices.clear();
	LODResource.Indices.clear();
	LODResource.Sections.clear();

	float IntervalX = InWidth / InNumSectionW;
	float IntervalY = InHeight / InNumSectionH;
	for (int i = 0; i <= InNumSectionW; ++i)
	{
		for (int j = 0; j <= InNumSectionH; ++j)
		{
			float X = IntervalX * i;
			float Y = IntervalY * j;
			LocalVertex V;
			V.Position = { X - InWidth * 0.5f, Y - InHeight * 0.5f, 0.0f };
			V.TangentZ = {0,0,1};
			LODResource.Vertices.push_back(V);

			PositionOnlyLocalVertex PLV;
			PLV.Position = { X - InWidth * 0.5f, Y - InHeight * 0.5f, 0.0f , 1.0f};
			LODResource.PositionOnlyVertices.push_back(PLV);
		}
	}

	int Count = InNumSectionW * InNumSectionH;
	for (int i = 0; i < Count;++i)
	{
		LODResource.Indices.push_back(i);
		LODResource.Indices.push_back(i + 1);
		LODResource.Indices.push_back(i + 1 + InNumSectionW + 1);
		LODResource.Indices.push_back(i);
		LODResource.Indices.push_back(i + 1 + InNumSectionW + 1);
		LODResource.Indices.push_back(i + InNumSectionW + 1);
	}

	StaticMeshSection Section;
	Section.FirstIndex = 0;
	Section.NumTriangles = Count * 2;
	LODResource.Sections.push_back(Section);

	LODResource.InitResource();
}

void StaticMesh::GnerateBox(float InSizeX, float InSizeY, float InSizeZ, int InNumSectionX, int InNumSectionY, int InNumSectionZ)
{

}
/** Initializes the primitive uniform shader parameters. */
inline PrimitiveUniform GetPrimitiveUniformShaderParameters(
	const Matrix& LocalToWorld,
	Vector ActorPosition,
	//const FBoxSphereBounds& WorldBounds,
	//const FBoxSphereBounds& LocalBounds,
	bool bReceivesDecals,
	bool bHasDistanceFieldRepresentation,
	bool bHasCapsuleRepresentation,
	bool bUseSingleSampleShadowFromStationaryLights,
	bool bUseVolumetricLightmap,
	bool bUseEditorDepthTest,
	uint32 LightingChannelMask,
	float LpvBiasMultiplier = 1.0f
)
{
	PrimitiveUniform Result;
	Result.LocalToWorld = LocalToWorld;
	Result.WorldToLocal = LocalToWorld.Inverse();
	//Result.ObjectWorldPositionAndRadius = Vector4(WorldBounds.Origin, WorldBounds.SphereRadius);
	//Result.ObjectBounds = WorldBounds.BoxExtent;
	//Result.LocalObjectBoundsMin = LocalBounds.GetBoxExtrema(0); // 0 == minimum
	//Result.LocalObjectBoundsMax = LocalBounds.GetBoxExtrema(1); // 1 == maximum
	//Result.ObjectOrientation = LocalToWorld.GetUnitAxis(EAxis::Z);
	Result.ActorWorldPosition = ActorPosition;
	Result.LightingChannelMask = LightingChannelMask;
	Result.LpvBiasMultiplier = LpvBiasMultiplier;

	{
		// Extract per axis scales from LocalToWorld transform
		Vector4 WorldX = Vector4(LocalToWorld.M[0][0], LocalToWorld.M[0][1], LocalToWorld.M[0][2], 0);
		Vector4 WorldY = Vector4(LocalToWorld.M[1][0], LocalToWorld.M[1][1], LocalToWorld.M[1][2], 0);
		Vector4 WorldZ = Vector4(LocalToWorld.M[2][0], LocalToWorld.M[2][1], LocalToWorld.M[2][2], 0);
		float ScaleX = Vector4(WorldX).Size();
		float ScaleY = Vector4(WorldY).Size();
		float ScaleZ = Vector4(WorldZ).Size();
		Result.NonUniformScale = Vector4(ScaleX, ScaleY, ScaleZ, 0);
		Result.InvNonUniformScale = Vector4(
			ScaleX > KINDA_SMALL_NUMBER ? 1.0f / ScaleX : 0.0f,
			ScaleY > KINDA_SMALL_NUMBER ? 1.0f / ScaleY : 0.0f,
			ScaleZ > KINDA_SMALL_NUMBER ? 1.0f / ScaleZ : 0.0f,
			0.0f
		);
	}

	//Result.LocalToWorldDeterminantSign = Math::FloatSelect(LocalToWorld.RotDeterminant(), 1, -1);
	Result.DecalReceiverMask = bReceivesDecals ? 1 : 0;
	Result.PerObjectGBufferData = (2 * (int32)bHasCapsuleRepresentation + (int32)bHasDistanceFieldRepresentation) / 3.0f;
	Result.UseSingleSampleShadowFromStationaryLights = bUseSingleSampleShadowFromStationaryLights ? 1.0f : 0.0f;
	Result.UseVolumetricLightmapShadowFromStationaryLights = bUseVolumetricLightmap && bUseSingleSampleShadowFromStationaryLights ? 1.0f : 0.0f;
	Result.UseEditorDepthTest = bUseEditorDepthTest ? 1 : 0;
	return Result;
}

void StaticMesh::InitResource()
{
	LODResource.InitResource();
	PrimitiveUniform PU = GetPrimitiveUniformShaderParameters(
		GetWorldMatrix(),
		Position,
		//Bounds,
		//LocalBounds,
		/*bReceivesDecals*/false,
		/*HasDistanceFieldRepresentation()*/false,
		/*HasDynamicIndirectShadowCasterRepresentation()*/false,
		/*UseSingleSampleShadowFromStationaryLights()*/false,
		/*Scene->HasPrecomputedVolumetricLightmap_RenderThread()*/false,
		/*UseEditorDepthTest()*/false,
		/*GetLightingChannelMask()*/0,
		/*LpvBiasMultiplier*/ 1.f
	);;
	PrimitiveUniformBuffer = CreateConstantBuffer(false,sizeof(PU),&PU);
	
}

void StaticMesh::ReleaseResource()
{
	LODResource.ReleaseResource();
	if(PrimitiveUniformBuffer) PrimitiveUniformBuffer->Release();
}

void StaticMesh::Register()
{
	InitResource();
	DrawStaticElement(GetWorld()->mScene);
}

void StaticMesh::UnRegister()
{
	ReleaseResource();
}


void StaticMesh::Tick(float fDeltaTime)
{
	UpdateUniformBuffer();
}

void StaticMesh::UpdateUniformBuffer()
{
	if (bTransformDirty)
	{
		PrimitiveUniform PU = GetPrimitiveUniformShaderParameters(
			GetWorldMatrix(),
			Position,
			//Bounds,
			//LocalBounds,
			/*bReceivesDecals*/false,
			/*HasDistanceFieldRepresentation()*/false,
			/*HasDynamicIndirectShadowCasterRepresentation()*/false,
			/*UseSingleSampleShadowFromStationaryLights()*/false,
			/*Scene->HasPrecomputedVolumetricLightmap_RenderThread()*/false,
			/*UseEditorDepthTest()*/false,
			/*GetLightingChannelMask()*/0,
			/*LpvBiasMultiplier*/ 1.f
		);
		D3D11DeviceContext->UpdateSubresource(PrimitiveUniformBuffer, 0, 0, &PU, 0, 0);
		bTransformDirty = false;
	}
}

bool StaticMesh::GetMeshElement(int BatchIndex, int SectionIndex, MeshBatch& OutMeshBatch)
{
	const StaticMeshSection& Section = LODResource.Sections[SectionIndex];
	MeshElement Element;
	Element.PrimitiveUniformBuffer = PrimitiveUniformBuffer;
	Element.FirstIndex = Section.FirstIndex;
	Element.NumTriangles = Section.NumTriangles;
	Element.MaterialIndex = Section.MaterialIndex;
	OutMeshBatch.Elements.emplace_back(Element);
	return true;
}

void StaticMesh::DrawStaticElement(class Scene* InScene)
{
	for (size_t SectionIndex = 0; SectionIndex < LODResource.Sections.size();++SectionIndex)
	{
		MeshBatch MB;
		MB.IndexBuffer = LODResource.IndexBuffer;
		MB.VertexBuffer = LODResource.VertexBuffer;
		MB.PositionOnlyVertexBuffer = LODResource.PositionOnlyVertexBuffer;

		for (int BatchIndex = 0; BatchIndex < GetNumberBatches(); ++BatchIndex)
		{
			if (GetMeshElement(BatchIndex, SectionIndex, MB))
			{
				InScene->AllBatches.emplace_back(MB);
			}
		}
	}
}

void StaticMesh::Build()
{

	MeshDescription MD2;

	GetRenderMeshDescription(MD, MD2);

	std::vector<std::vector<uint32> > OutPerSectionIndices;
	OutPerSectionIndices.resize(MD.PolygonGroups().Num());
	std::vector<StaticMeshBuildVertex> StaticMeshBuildVertices;
	BuildVertexBuffer(MD2, OutPerSectionIndices, StaticMeshBuildVertices);

	std::vector<uint32> CombinedIndices;
	for (uint32 i = 0; i < LODResource.Sections.size(); ++i)
	{
		StaticMeshSection& Section = LODResource.Sections[i];
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
	for (const auto& V : StaticMeshBuildVertices)
	{
		LocalVertex LV;
		LV.Position = Vector4(V.Position,1.0f);
		LV.TangentX = V.TangentX;
		LV.TangentZ = Vector4(V.TangentZ, V.TangentYSign);
		LV.TexCoords = V.UVs;
		LV.LightMapCoordinate = V.LightMapCoordinate;
		LODResource.Vertices.push_back(LV);

		PositionOnlyLocalVertex PLV;
		PLV.Position = Vector4(V.Position,1.0f);
		LODResource.PositionOnlyVertices.push_back(PLV);
	}
}

void StaticMesh::BuildVertexBuffer(const MeshDescription& MD2, std::vector<std::vector<uint32> >& OutPerSectionIndices, std::vector<StaticMeshBuildVertex>& StaticMeshBuildVertices)
{
	const TMeshElementArray<MeshVertex>& Vertices = MD2.Vertices();
	const TMeshElementArray<MeshVertexInstance>& VertexInstances = MD2.VertexInstances();
	const TMeshElementArray<MeshPolygonGroup>& PolygonGroupArray = MD2.PolygonGroups();
	const TMeshElementArray<MeshPolygon>& PolygonArray = MD2.Polygons();

	const std::vector<std::string>& PolygonGroupImportedMaterialSlotNames = MD2.PolygonGroupAttributes().GetAttributes<std::string>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

	const std::vector<Vector>& VertexPositions = MD2.VertexAttributes().GetAttributes<Vector>(MeshAttribute::Vertex::Position);
	const std::vector<Vector>& VertexInstanceNormals = MD2.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Normal);
	const std::vector<Vector>& VertexInstanceTangents = MD2.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Tangent);
	const std::vector<float>& VertexInstanceBinormalSigns = MD2.VertexInstanceAttributes().GetAttributes<float>(MeshAttribute::VertexInstance::BinormalSign);
	const std::vector<Vector4>& VertexInstanceColors = MD2.VertexInstanceAttributes().GetAttributes<Vector4>(MeshAttribute::VertexInstance::Color);
	const std::vector<std::vector<Vector2>>& VertexInstanceUVs = MD2.VertexInstanceAttributes().GetAttributesSet<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);

	std::map<int, int> PolygonGroupToSectionIndex;
	for (const int PolgyonGroupID : MD2.PolygonGroups().GetElementIDs())
	{
		int& SectionIndex = PolygonGroupToSectionIndex[PolgyonGroupID];
		LODResource.Sections.push_back(StaticMeshSection());
		SectionIndex = (int)LODResource.Sections.size() - 1;
		StaticMeshSection& Section = LODResource.Sections[SectionIndex];
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
	LODResource.Indices.reserve(ReserveIndicesCount);

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
			Vector CornerPositions[3];
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
				const Vector& VertexPosition = CornerPositions[TriVert];
				const Vector& VertexNormal = VertexInstanceNormals[VertexInstanceID];
				const Vector& VertexTangent = VertexInstanceTangents[VertexInstanceID];
				const float VertexInstanceBinormalSign = VertexInstanceBinormalSigns[VertexInstanceID];
				Vector2 UVs = VertexInstanceUVs[0][VertexInstanceID];
				Vector2 LightMapCoordinate = VertexInstanceUVs[1][VertexInstanceID];

				StaticMeshBuildVertex StaticMeshVertex;
				StaticMeshVertex.Position = VertexPosition;
				StaticMeshVertex.TangentX = VertexTangent;
				StaticMeshVertex.TangentYSign =  VertexInstanceBinormalSign;
				StaticMeshVertex.TangentZ = VertexNormal;
				StaticMeshVertex.UVs = UVs;
				StaticMeshVertex.LightMapCoordinate = LightMapCoordinate;

				StaticMeshBuildVertices.push_back(StaticMeshVertex);
				SectionIndices.push_back(StaticMeshBuildVertices.size() - 1) ;
			}
		}

	}
}

void StaticMesh::GetRenderMeshDescription(const MeshDescription& InOriginalMeshDescription, MeshDescription& OutRenderMeshDescription)
{
	OutRenderMeshDescription = InOriginalMeshDescription;

	float ComparisonThreshold = 0.00002f;//BuildSettings->bRemoveDegenerates ? THRESH_POINTS_ARE_SAME : 0.0f;

	MeshDescriptionOperations::CreatePolygonNTB(OutRenderMeshDescription, 0.f);

	TMeshElementArray<MeshVertexInstance>& VertexInstanceArray = OutRenderMeshDescription.VertexInstances();
	std::vector<Vector>& Normals = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Normal);
	std::vector<Vector>& Tangents = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Tangent);
	std::vector<float>& BinormalSigns = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<float>(MeshAttribute::VertexInstance::BinormalSign);

	MeshDescriptionOperations::FindOverlappingCorners(OverlappingCorners, OutRenderMeshDescription, ComparisonThreshold);

	uint32 TangentOptions = MeshDescriptionOperations::ETangentOptions::BlendOverlappingNormals;

	bool bHasAllNormals = true;
	bool bHasAllTangents = true;

	for (const int VertexInstanceID : VertexInstanceArray.GetElementIDs())
	{
		bHasAllNormals &= !Normals[VertexInstanceID].IsNearlyZero();
		bHasAllTangents &= !Tangents[VertexInstanceID].IsNearlyZero();
	}

	if (!bHasAllNormals)
	{

	}

	MeshDescriptionOperations::CreateMikktTangents(OutRenderMeshDescription, (MeshDescriptionOperations::ETangentOptions)TangentOptions);

	
	std::vector<std::vector<Vector2>>& VertexInstanceUVs = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributesSet<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);
	int32 NumIndices = (int32)VertexInstanceUVs.size();
	//Verify the src light map channel
// 	if (BuildSettings->SrcLightmapIndex >= NumIndices)
// 	{
// 		BuildSettings->SrcLightmapIndex = 0;
// 	}
	//Verify the destination light map channel
// 	if (BuildSettings->DstLightmapIndex >= NumIndices)
// 	{
// 		//Make sure we do not add illegal UV Channel index
// 		if (BuildSettings->DstLightmapIndex >= MAX_MESH_TEXTURE_COORDS_MD)
// 		{
// 			BuildSettings->DstLightmapIndex = MAX_MESH_TEXTURE_COORDS_MD - 1;
// 		}
// 
// 		//Add some unused UVChannel to the mesh description for the lightmapUVs
// 		VertexInstanceUVs.SetNumIndices(BuildSettings->DstLightmapIndex + 1);
// 		BuildSettings->DstLightmapIndex = NumIndices;
// 	}
	VertexInstanceUVs.resize(2);
	VertexInstanceUVs[1].resize(VertexInstanceUVs[0].size());
	MeshDescriptionOperations::CreateLightMapUVLayout(OutRenderMeshDescription,
		/*BuildSettings->SrcLightmapIndex,*/0,
		/*BuildSettings->DstLightmapIndex,*/1,
		/*BuildSettings->MinLightmapResolution,*/64,
		/*(MeshDescriptionOperations::ELightmapUVVersion)(StaticMesh->LightmapUVVersion),*/(MeshDescriptionOperations::ELightmapUVVersion)4,
		OverlappingCorners);
}
