#include "MeshDescriptionOperations.h"
#include <vector>
#include "UnrealMath.h"
#include "MeshDescription.h"
#include "mikktspace.h"
#include "LayoutUV.h"
#include "Allocator2D.h"
#include <algorithm>

template <typename T>
void AddUnique(std::vector<T>& container, const T& Value);
template <typename T>
bool IsValidIndex(std::vector<T>& container, int32 Index);
template <typename T>
bool Contains(std::vector<T>& container, const T& Value)
{
	return std::find(container.begin(), container.end(), Value) != container.end();
}
/** Helper struct for building acceleration structures. */
namespace MeshDescriptionOperationNamespace
{
	struct FIndexAndZ
	{
		float Z;
		int32 Index;
		const FVector *OriginalVector;

		/** Default constructor. */
		FIndexAndZ() {}

		/** Initialization constructor. */
		FIndexAndZ(int32 InIndex, const FVector& V)
		{
			Z = 0.30f * V.X + 0.33f * V.Y + 0.37f * V.Z;
			Index = InIndex;
			OriginalVector = &V;
		}
	};
	/** Sorting function for vertex Z/index pairs. */
	struct FCompareIndexAndZ
	{
		inline bool operator()(FIndexAndZ const& A, FIndexAndZ const& B) const { return A.Z < B.Z; }
	};
}
struct FVertexInfo
{
	FVertexInfo()
	{
		PolygonID = -1;
		VertexInstanceID = -1;
		UVs = Vector2(0.0f, 0.0f);
		EdgeIDs.reserve(2);//Most of the time a edge has two triangles
	}

	int32 PolygonID;
	int32 VertexInstanceID;
	Vector2 UVs;
	std::vector<int32> EdgeIDs;
};

void MeshDescriptionOperations::CreatePolygonNTB(MeshDescription& MD, float ComparisonThreshold)
{
	const std::vector<FVector>& VertexPositions = MD.VertexAttributes().GetAttributes<FVector>(MeshAttribute::Vertex::Position);
	std::vector<Vector2>& VertexUVs = MD.VertexInstanceAttributes().GetAttributes<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, 0);
	std::vector<FVector>& PolygonNormals = MD.PolygonAttributes().GetAttributes<FVector>(MeshAttribute::Polygon::Normal);
	std::vector<FVector>& PolygonTangents = MD.PolygonAttributes().GetAttributes<FVector>(MeshAttribute::Polygon::Tangent);
	std::vector<FVector>& PolygonBinormals = MD.PolygonAttributes().GetAttributes<FVector>(MeshAttribute::Polygon::Binormal);

	TMeshElementArray<MeshVertexInstance>& VertexInstanceArray = MD.VertexInstances();
	TMeshElementArray<MeshVertex>& VertexArray = MD.Vertices();
	TMeshElementArray<MeshPolygon>& PolygonArray = MD.Polygons();

	for (const int PolygonID : MD.Polygons().GetElementIDs())
	{
		if (!PolygonNormals[PolygonID].IsNearlyZero())
		{
			continue;
		}

		const std::vector<MeshTriangle> MeshTriangles = MD.GetPolygonTriangles(PolygonID);
		FVector TangentX(0.0f);
		FVector TangentY(0.0f);
		FVector TangentZ(0.0f);

		for (const MeshTriangle& MT : MeshTriangles)
		{
			int UVIndex = 0;

			FVector P[3];
			Vector2 UVs[3];

			for (int32 i = 0; i < 3; ++i)
			{
				const int VertexInstanceID = MT.GetVertexInstanceID(i);
				UVs[i] = VertexUVs[VertexInstanceID];
				P[i] = VertexPositions[MD.GetVertexInstanceVertex(VertexInstanceID)];
			}

			const FVector Normal = ((P[1] - P[2]) ^ (P[0] - P[2])).GetSafeNormal(ComparisonThreshold);
			if (!Normal.IsNearlyZero(ComparisonThreshold))
			{
				FMatrix ParameterToLocal(
					FPlane(P[1].X - P[0].X, P[1].Y - P[0].Y, P[1].Z - P[0].Z, 0),
					FPlane(P[2].X - P[0].X, P[2].Y - P[0].Y, P[2].Z - P[0].Z, 0),
					FPlane(P[0].X, P[0].Y, P[0].Z, 0),
					FPlane(0, 0, 0, 1)
				);

				FMatrix ParameterToTexture(
					FPlane(UVs[1].X - UVs[0].X, UVs[1].Y - UVs[0].Y, 0, 0),
					FPlane(UVs[2].X - UVs[0].X, UVs[2].Y - UVs[0].Y, 0, 0),
					FPlane(UVs[0].X, UVs[0].Y, 1, 0),
					FPlane(0, 0, 0, 1)
				);

				// Use InverseSlow to catch singular matrices.  Inverse can miss this sometimes.
				const FMatrix TextureToLocal = ParameterToTexture.Inverse() * ParameterToLocal;

				FVector TmpTangentX(0.0f);
				FVector TmpTangentY(0.0f);
				FVector TmpTangentZ(0.0f);
				TmpTangentX = TextureToLocal.Transform(FVector(1, 0, 0)).GetSafeNormal();
				TmpTangentY = TextureToLocal.Transform(FVector(0, 1, 0)).GetSafeNormal();
				TmpTangentZ = Normal;
				FVector::CreateOrthonormalBasis(TmpTangentX, TmpTangentY, TmpTangentZ);
				TangentX += TmpTangentX;
				TangentY += TmpTangentY;
				TangentZ += TmpTangentZ;
			}
			else
			{
				//This will force a recompute of the normals and tangents
				TangentX = FVector(0.0f);
				TangentY = FVector(0.0f);
				TangentZ = FVector(0.0f);
				break;
			}
			TangentX.Normalize();
			TangentY.Normalize();
			TangentZ.Normalize();
			PolygonTangents[PolygonID] = TangentX;
			PolygonBinormals[PolygonID] = TangentY;
			PolygonNormals[PolygonID] = TangentZ;
		}
	}
}

void MeshDescriptionOperations::CreateNormals(MeshDescription& MD, ETangentOptions TangentOptions, bool bComputeTangent)
{
	//For each vertex compute the normals for every connected edges that are smooth betwween hard edges
	//         H   A    B
	//          \  ||  /
	//       G  -- ** -- C
	//          // |  \
			//         F   E    D
//
// The double ** are the vertex, the double line are hard edges, the single line are soft edge.
// A and F are hard, all other edges are soft. The goal is to compute two average normals one from
// A to F and a second from F to A. Then we can set the vertex instance normals accordingly.
// First normal(A to F) = Normalize(A+B+C+D+E+F)
// Second normal(F to A) = Normalize(F+G+H+A)
// We found the connected edge using the triangle that share edges

// @todo: provide an option to weight each contributing polygon normal according to the size of
// the angle it makes with the vertex being calculated. This means that triangulated faces whose
// internal edge meets the vertex doesn't get undue extra weight.

	const std::vector<Vector2>& VertexUVs = MD.VertexInstanceAttributes().GetAttributes<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, 0);
	std::vector<FVector>& VertexNormals = MD.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Normal, 0);
	std::vector<FVector>& VertexTangents = MD.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Tangent, 0);
	std::vector<float>& VertexBinormalSigns = MD.VertexInstanceAttributes().GetAttributes<float>(MeshAttribute::VertexInstance::BinormalSign, 0);

	std::vector<FVector>& PolygonNormals = MD.PolygonAttributes().GetAttributes<FVector>(MeshAttribute::Polygon::Normal);
	std::vector<FVector>& PolygonTangents = MD.PolygonAttributes().GetAttributes<FVector>(MeshAttribute::Polygon::Tangent);
	std::vector<FVector>& PolygonBinormals = MD.PolygonAttributes().GetAttributes<FVector>(MeshAttribute::Polygon::Binormal);

	std::map<int32, FVertexInfo> VertexInfoMap;
	//Iterate all vertex to compute normals for all vertex instance
	for (const int32 VertexID : MD.Vertices().GetElementIDs())
	{
		VertexInfoMap.clear();

		bool bPointHasAllTangents = true;
		//Fill the VertexInfoMap
		for (const int32 EdgeID : MD.GetVertexConnectedEdges(VertexID))
		{
			for (const int32 PolygonID : MD.GetEdgeConnectedPolygons(EdgeID))
			{
				FVertexInfo& VertexInfo = VertexInfoMap[PolygonID];
				int32 EdgeIndex = VertexInfo.EdgeIDs.size();
				AddUnique(VertexInfo.EdgeIDs, EdgeID);
				if (VertexInfo.PolygonID == -1)
				{
					VertexInfo.PolygonID = PolygonID;
					for (const int32 VertexInstanceID : MD.GetPolygonPerimeterVertexInstances(PolygonID))
					{
						if (MD.GetVertexInstanceVertex(VertexInstanceID) == VertexID)
						{
							VertexInfo.VertexInstanceID = VertexInstanceID;
							VertexInfo.UVs = VertexUVs[VertexInstanceID];
							bPointHasAllTangents &= !VertexNormals[VertexInstanceID].IsNearlyZero() && !VertexTangents[VertexInstanceID].IsNearlyZero();
							if (bPointHasAllTangents)
							{
								FVector TangentX = VertexTangents[VertexInstanceID].GetSafeNormal();
								FVector TangentZ = VertexNormals[VertexInstanceID].GetSafeNormal();
								FVector TangentY = (FVector::CrossProduct(TangentZ, TangentX).GetSafeNormal() * VertexBinormalSigns[VertexInstanceID]).GetSafeNormal();
								if (TangentX.ContainsNaN() || TangentX.IsNearlyZero(SMALL_NUMBER) ||
									TangentY.ContainsNaN() || TangentY.IsNearlyZero(SMALL_NUMBER) ||
									TangentZ.ContainsNaN() || TangentZ.IsNearlyZero(SMALL_NUMBER))
								{
									bPointHasAllTangents = false;
								}
							}
							break;
						}
					}
				}
			}
		}

		if (bPointHasAllTangents)
		{
			continue;
		}

		//Build all group by recursively traverse all polygon connected to the vertex
		std::vector<std::vector<int32>> Groups;
		std::vector<int32> ConsumedPolygon;
		for (auto Kvp : VertexInfoMap)
		{
			if (Contains(ConsumedPolygon, Kvp.first))
			{
				continue;
			}

			int32 CurrentGroupIndex = Groups.size();
			Groups.push_back(std::vector<int32>());
			std::vector<int32>& CurrentGroup = Groups[CurrentGroupIndex];
			std::vector<int32> PolygonQueue;
			PolygonQueue.push_back(Kvp.first); //Use a queue to avoid recursive function
			while (PolygonQueue.size() > 0)
			{
				int32 CurrentPolygonID = PolygonQueue.back();
				PolygonQueue.pop_back();
				FVertexInfo& CurrentVertexInfo = VertexInfoMap[CurrentPolygonID];
				AddUnique(CurrentGroup, CurrentVertexInfo.PolygonID);
				AddUnique(ConsumedPolygon,CurrentVertexInfo.PolygonID);
				const std::vector<bool>& EdgeHardnesses = MD.EdgeAttributes().GetAttributes<bool>(MeshAttribute::Edge::IsHard);
				for (const int32 EdgeID : CurrentVertexInfo.EdgeIDs)
				{
					if (EdgeHardnesses[EdgeID])
					{
						//End of the group
						continue;
					}
					for (const int32 PolygonID : MD.GetEdgeConnectedPolygons(EdgeID))
					{
						if (PolygonID == CurrentVertexInfo.PolygonID)
						{
							continue;
						}
						//Add this polygon to the group
						FVertexInfo& OtherVertexInfo = VertexInfoMap[PolygonID];
						//Do not repeat polygons
						if (!Contains(ConsumedPolygon,OtherVertexInfo.PolygonID))
						{
							PolygonQueue.push_back(PolygonID);
						}
					}
				}
			}
		}

		//Smooth every connected group
		ConsumedPolygon.clear();
		for (const std::vector<int32>& Group : Groups)
		{
			//Compute tangents data
			std::map<Vector2, FVector> GroupTangent;
			std::map<Vector2, FVector> GroupBiNormal;

			std::vector<int32> VertexInstanceInGroup;
			FVector GroupNormal(0.0f);
			for (const int32 PolygonID : Group)
			{
				FVector PolyNormal = PolygonNormals[PolygonID];
				FVector PolyTangent = PolygonTangents[PolygonID];
				FVector PolyBinormal = PolygonBinormals[PolygonID];

				ConsumedPolygon.push_back(PolygonID);
				VertexInstanceInGroup.push_back(VertexInfoMap[PolygonID].VertexInstanceID);
				if (!PolyNormal.IsNearlyZero(SMALL_NUMBER) && !PolyNormal.ContainsNaN())
				{
					GroupNormal += PolyNormal;
				}
				if (bComputeTangent)
				{
					const Vector2 UVs = VertexInfoMap[PolygonID].UVs;
					bool CreateGroup = (GroupTangent.find(UVs) == GroupTangent.end());
					FVector& GroupTangentValue = GroupTangent[UVs];
					FVector& GroupBiNormalValue = GroupBiNormal[UVs];
					if (CreateGroup)
					{
						GroupTangentValue = FVector(0.0f);
						GroupBiNormalValue = FVector(0.0f);
					}
					if (!PolyTangent.IsNearlyZero(SMALL_NUMBER) && !PolyTangent.ContainsNaN())
					{
						GroupTangentValue += PolyTangent;
					}
					if (!PolyBinormal.IsNearlyZero(SMALL_NUMBER) && !PolyBinormal.ContainsNaN())
					{
						GroupBiNormalValue += PolyBinormal;
					}
				}
			}

			//////////////////////////////////////////////////////////////////////////
			//Apply the group to the Mesh
			GroupNormal.Normalize();
			if (bComputeTangent)
			{
				for (auto Kvp : GroupTangent)
				{
					FVector& GroupTangentValue = GroupTangent[Kvp.first];
					GroupTangentValue.Normalize();
				}
				for (auto Kvp : GroupBiNormal)
				{
					FVector& GroupBiNormalValue = GroupBiNormal[Kvp.first];
					GroupBiNormalValue.Normalize();
				}
			}
			//Apply the average NTB on all Vertex instance
			for (const int32 VertexInstanceID : VertexInstanceInGroup)
			{
				const Vector2 VertexUV = VertexUVs[VertexInstanceID];

				if (VertexNormals[VertexInstanceID].IsNearlyZero(SMALL_NUMBER))
				{
					VertexNormals[VertexInstanceID] = GroupNormal;
				}
				if (bComputeTangent)
				{
					//Avoid changing the original group value
					FVector GroupTangentValue = GroupTangent[VertexUV];
					FVector GroupBiNormalValue = GroupBiNormal[VertexUV];

					if (!VertexTangents[VertexInstanceID].IsNearlyZero(SMALL_NUMBER))
					{
						GroupTangentValue = VertexTangents[VertexInstanceID];
					}
					FVector BiNormal(0.0f);
					if (!VertexNormals[VertexInstanceID].IsNearlyZero(SMALL_NUMBER) && !VertexTangents[VertexInstanceID].IsNearlyZero(SMALL_NUMBER))
					{
						BiNormal = FVector::CrossProduct(VertexNormals[VertexInstanceID], VertexTangents[VertexInstanceID]).GetSafeNormal() * VertexBinormalSigns[VertexInstanceID];
					}
					if (!BiNormal.IsNearlyZero(SMALL_NUMBER))
					{
						GroupBiNormalValue = BiNormal;
					}
					// Gram-Schmidt orthogonalization
					GroupBiNormalValue -= GroupTangentValue * (GroupTangentValue | GroupBiNormalValue);
					GroupBiNormalValue.Normalize();

					GroupTangentValue -= VertexNormals[VertexInstanceID] * (VertexNormals[VertexInstanceID] | GroupTangentValue);
					GroupTangentValue.Normalize();

					GroupBiNormalValue -= VertexNormals[VertexInstanceID] * (VertexNormals[VertexInstanceID] | GroupBiNormalValue);
					GroupBiNormalValue.Normalize();
					//Set the value
					VertexTangents[VertexInstanceID] = GroupTangentValue;
					//If the BiNormal is zero set the sign to 1.0f
					VertexBinormalSigns[VertexInstanceID] = GetBasisDeterminantSign(GroupTangentValue, GroupBiNormalValue, VertexNormals[VertexInstanceID]);

				}
			}
		}
	}
}

namespace MeshDescriptionMikktSpaceInterface
{
	//Mikk t spce static function
	int MikkGetNumFaces(const SMikkTSpaceContext* Context);
	int MikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx);
	void MikkGetPosition(const SMikkTSpaceContext* Context, float Position[3], const int FaceIdx, const int VertIdx);
	void MikkGetNormal(const SMikkTSpaceContext* Context, float Normal[3], const int FaceIdx, const int VertIdx);
	void MikkSetTSpaceBasic(const SMikkTSpaceContext* Context, const float Tangent[3], const float BitangentSign, const int FaceIdx, const int VertIdx);
	void MikkGetTexCoord(const SMikkTSpaceContext* Context, float UV[2], const int FaceIdx, const int VertIdx);
}


void MeshDescriptionOperations::CreateMikktTangents(MeshDescription& MD, ETangentOptions TangentOptions)
{
	bool bIgnoreDegenerateTriangles = (TangentOptions & MeshDescriptionOperations::ETangentOptions::IgnoreDegenerateTriangles) != 0;
	SMikkTSpaceInterface MikkTInterface;
	MikkTInterface.m_getNormal = MeshDescriptionMikktSpaceInterface::MikkGetNormal;
	MikkTInterface.m_getNumFaces = MeshDescriptionMikktSpaceInterface::MikkGetNumFaces;
	MikkTInterface.m_getNumVerticesOfFace = MeshDescriptionMikktSpaceInterface::MikkGetNumVertsOfFace;
	MikkTInterface.m_getPosition = MeshDescriptionMikktSpaceInterface::MikkGetPosition;
	MikkTInterface.m_getTexCoord = MeshDescriptionMikktSpaceInterface::MikkGetTexCoord;
	MikkTInterface.m_setTSpaceBasic = MeshDescriptionMikktSpaceInterface::MikkSetTSpaceBasic;
	MikkTInterface.m_setTSpace = nullptr;

	SMikkTSpaceContext MikkTContext;
	MikkTContext.m_pInterface = &MikkTInterface;
	MikkTContext.m_pUserData = (void*)(&MD);
	MikkTContext.m_bIgnoreDegenerates = bIgnoreDegenerateTriangles;
	genTangSpaceDefault(&MikkTContext);
}

void MeshDescriptionOperations::FindOverlappingCorners(std::multimap<int, int>& OverlappingCorners, const MeshDescription& MD, float ComparisonThreshold)
{
	//Empty the old data
	OverlappingCorners.clear();

	const TMeshElementArray<MeshVertexInstance>& VertexInstanceArray = MD.VertexInstances();
	const TMeshElementArray<MeshVertex>& VertexArray = MD.Vertices();

	const int32 NumWedges = VertexInstanceArray.Num();

	// Create a list of vertex Z/index pairs
	std::vector<MeshDescriptionOperationNamespace::FIndexAndZ> VertIndexAndZ;
	VertIndexAndZ.reserve(NumWedges);

	const std::vector<FVector>& VertexPositions = MD.VertexAttributes().GetAttributes<FVector>(MeshAttribute::Vertex::Position);

	for (const int VertexInstanceID : VertexInstanceArray.GetElementIDs())
	{
		VertIndexAndZ.push_back(MeshDescriptionOperationNamespace::FIndexAndZ(VertexInstanceID, VertexPositions[MD.GetVertexInstanceVertex(VertexInstanceID)]));
		//new(VertIndexAndZ)MeshDescriptionOperationNamespace::FIndexAndZ(VertexInstanceID, VertexPositions[MD.GetVertexInstanceVertex(VertexInstanceID)]);
	}

	// Sort the vertices by z value
	std::sort(VertIndexAndZ.begin(), VertIndexAndZ.end(), MeshDescriptionOperationNamespace::FCompareIndexAndZ());
	//VertIndexAndZ.Sort(MeshDescriptionOperationNamespace::FCompareIndexAndZ());

	// Search for duplicates, quickly!
	for (size_t i = 0; i < VertIndexAndZ.size(); i++)
	{
		// only need to search forward, since we add pairs both ways
		for (size_t j = i + 1; j < VertIndexAndZ.size(); j++)
		{
			if (FMath::Abs(VertIndexAndZ[j].Z - VertIndexAndZ[i].Z) > ComparisonThreshold)
				break; // can't be any more dups

			const FVector& PositionA = *(VertIndexAndZ[i].OriginalVector);
			const FVector& PositionB = *(VertIndexAndZ[j].OriginalVector);

			if (PositionA.Equals(PositionB, ComparisonThreshold))
			{
				OverlappingCorners.insert(std::make_pair(VertIndexAndZ[i].Index, VertIndexAndZ[j].Index));
				OverlappingCorners.insert(std::make_pair(VertIndexAndZ[j].Index, VertIndexAndZ[i].Index));
			}
		}
	}
}

namespace MeshDescriptionMikktSpaceInterface
{
	int MikkGetNumFaces(const SMikkTSpaceContext* Context)
	{
		MeshDescription* MD = (MeshDescription*)(Context->m_pUserData);
		return MD->Polygons().Num();
	}

	int MikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx)
	{
		// All of our meshes are triangles.
		MeshDescription* MD = (MeshDescription*)(Context->m_pUserData);
		const MeshPolygon& Polygon = MD->GetPolygon(FaceIdx);
		return Polygon.PerimeterContour.VertexInstanceIDs.size();
	}

	void MikkGetPosition(const SMikkTSpaceContext* Context, float Position[3], const int FaceIdx, const int VertIdx)
	{
		MeshDescription* MD = (MeshDescription*)(Context->m_pUserData);
		const MeshPolygon& Polygon = MD->GetPolygon(FaceIdx);
		const int VertexInstanceID = Polygon.PerimeterContour.VertexInstanceIDs[VertIdx];
		const int VertexID = MD->GetVertexInstanceVertex(VertexInstanceID);
		const FVector& VertexPosition = MD->VertexAttributes().GetAttribute<FVector>(VertexID, MeshAttribute::Vertex::Position);
		Position[0] = VertexPosition.X;
		Position[1] = VertexPosition.Y;
		Position[2] = VertexPosition.Z;
	}

	void MikkGetNormal(const SMikkTSpaceContext* Context, float Normal[3], const int FaceIdx, const int VertIdx)
	{
		MeshDescription* MD = (MeshDescription*)(Context->m_pUserData);
		const MeshPolygon& Polygon = MD->GetPolygon(FaceIdx);
		const int VertexInstanceID = Polygon.PerimeterContour.VertexInstanceIDs[VertIdx];
		const FVector& VertexNormal = MD->VertexInstanceAttributes().GetAttribute<FVector>(VertexInstanceID, MeshAttribute::VertexInstance::Normal);
		Normal[0] = VertexNormal.X;
		Normal[1] = VertexNormal.Y;
		Normal[2] = VertexNormal.Z;
	}

	void MikkSetTSpaceBasic(const SMikkTSpaceContext* Context, const float Tangent[3], const float BitangentSign, const int FaceIdx, const int VertIdx)
	{
		MeshDescription* MD = (MeshDescription*)(Context->m_pUserData);
		const MeshPolygon& Polygon = MD->GetPolygon(FaceIdx);
		const int VertexInstanceID = Polygon.PerimeterContour.VertexInstanceIDs[VertIdx];
		const FVector VertexTangent(Tangent[0], Tangent[1], Tangent[2]);
		MD->VertexInstanceAttributes().SetAttribute<FVector>(VertexInstanceID, MeshAttribute::VertexInstance::Tangent, 0, VertexTangent);
		MD->VertexInstanceAttributes().SetAttribute<float>(VertexInstanceID, MeshAttribute::VertexInstance::BinormalSign, 0, -BitangentSign);
	}

	void MikkGetTexCoord(const SMikkTSpaceContext* Context, float UV[2], const int FaceIdx, const int VertIdx)
	{
		MeshDescription* MD = (MeshDescription*)(Context->m_pUserData);
		const MeshPolygon& Polygon = MD->GetPolygon(FaceIdx);
		const int VertexInstanceID = Polygon.PerimeterContour.VertexInstanceIDs[VertIdx];
		const Vector2& TexCoord = MD->VertexInstanceAttributes().GetAttribute<Vector2>(VertexInstanceID, MeshAttribute::VertexInstance::TextureCoordinate, 0);
		UV[0] = TexCoord.X;
		UV[1] = TexCoord.Y;
	}
}

void MeshDescriptionOperations::CreateLightMapUVLayout(MeshDescription& MD, int SrcLightmapIndex, int DstLightmapIndex, int MinLightmapResolution, ELightmapUVVersion LightmapUVVersion, const std::multimap<int, int>& OverlappingCorners)
{
	MeshDescriptionOp::FLayoutUV Packer(MD, SrcLightmapIndex, DstLightmapIndex, MinLightmapResolution);
	Packer.SetVersion(LightmapUVVersion);

	Packer.FindCharts(OverlappingCorners);
	bool bPackSuccess = Packer.FindBestPacking();
	if (bPackSuccess)
	{
		Packer.CommitPackedUVs();
	}
}

