#include "MeshDescriptionOperations.h"
#include <vector>
#include "UnrealMath.h"
#include "MeshDescription.h"
#include "mikktspace.h"
#include "LayoutUV.h"
#include "Allocator2D.h"
#include <algorithm>

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
		if (PolygonNormals[PolygonID].IsNearlyZero())
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

