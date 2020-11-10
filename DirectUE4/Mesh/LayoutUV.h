#pragma once

#include "UnrealMath.h"
#include "MeshDescription.h"
#include "MeshDescriptionOperations.h"
#include "Allocator2D.h"

#include <map>
#define THRESH_POINTS_ARE_SAME			(0.00002f)
#define THRESH_NORMALS_ARE_SAME			(0.00002f)
#define NEW_UVS_ARE_SAME THRESH_POINTS_ARE_SAME
#define LEGACY_UVS_ARE_SAME (1.0f / 1024.0f)
namespace MeshDescriptionOp
{
	struct FMeshChart
	{
		uint32		FirstTri;
		uint32		LastTri;

		Vector2	MinUV;
		Vector2	MaxUV;

		float		UVArea;
		Vector2	UVScale;
		Vector2	WorldScale;

		Vector2	PackingScaleU;
		Vector2	PackingScaleV;
		Vector2	PackingBias;

		int32		Join[4];
	};

	struct FAllocator2DShader
	{
		FAllocator2D*	Allocator2D;

		FAllocator2DShader(FAllocator2D* InAllocator2D)
			: Allocator2D(InAllocator2D)
		{}

		inline void Process(uint32 x, uint32 y)
		{
			Allocator2D->SetBit(x, y);
		}
	};

	class FLayoutUV
	{
	public:
		FLayoutUV(MeshDescription& InMesh, uint32 InSrcChannel, uint32 InDstChannel, uint32 InTextureResolution);

		void		FindCharts(const std::multimap<int32, int32>& OverlappingCorners);
		bool		FindBestPacking();
		void		CommitPackedUVs();

		void		SetVersion(MeshDescriptionOperations::ELightmapUVVersion Version) { LayoutVersion = Version; }

	private:
		bool		PositionsMatch(uint32 a, uint32 b) const;
		bool		NormalsMatch(uint32 a, uint32 b) const;
		bool		UVsMatch(uint32 a, uint32 b) const;
		bool		VertsMatch(uint32 a, uint32 b) const;
		float		TriangleUVArea(uint32 Tri) const;
		void		DisconnectChart(FMeshChart& Chart, uint32 Side);

		void		ScaleCharts(float UVScale);
		bool		PackCharts();
		void		OrientChart(FMeshChart& Chart, int32 Orientation);
		void		RasterizeChart(const FMeshChart& Chart, uint32 RectW, uint32 RectH);

		float		GetUVEqualityThreshold() const { return LayoutVersion >= MeshDescriptionOperations::ELightmapUVVersion::SmallChartPacking ? NEW_UVS_ARE_SAME : LEGACY_UVS_ARE_SAME; }

		MeshDescription&	MD;
		uint32				SrcChannel;
		uint32				DstChannel;
		uint32				TextureResolution;

		std::vector< Vector2 >		TexCoords;
		std::vector< uint32 >		SortedTris;
		std::vector< FMeshChart >	Charts;
		float					TotalUVArea;
		float					MaxChartSize;
		std::vector< int32 >	RemapVerts;

		FAllocator2D		LayoutRaster;
		FAllocator2D		ChartRaster;
		FAllocator2D		BestChartRaster;
		FAllocator2DShader	ChartShader;

		MeshDescriptionOperations::ELightmapUVVersion LayoutVersion;
	};


	inline bool FLayoutUV::PositionsMatch(uint32 a, uint32 b) const
	{
		const int VertexInstanceIDA(RemapVerts[a]);
		const int VertexInstanceIDB(RemapVerts[b]);
		const int VertexIDA = MD.GetVertexInstanceVertex(VertexInstanceIDA);
		const int VertexIDB = MD.GetVertexInstanceVertex(VertexInstanceIDB);

		const std::vector<Vector>& VertexPositions = MD.VertexAttributes().GetAttributes<Vector>(MeshAttribute::Vertex::Position);
		return VertexPositions[VertexIDA].Equals(VertexPositions[VertexIDB], THRESH_POINTS_ARE_SAME);
	}

	inline bool FLayoutUV::NormalsMatch(uint32 a, uint32 b) const
	{
		// If current SrcChannel is out of range of the number of UVs defined by the mesh description, just return true
		// @todo: hopefully remove this check entirely and just ensure that the mesh description matches the inputs
		const uint32 NumUVs = MD.VertexInstanceAttributes().GetAttributeIndexCount<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);
		if (SrcChannel >= NumUVs)
		{
			//ensure(false);	// not expecting it to get here
			return true;
		}

		const int VertexInstanceIDA(RemapVerts[a]);
		const int VertexInstanceIDB(RemapVerts[b]);

		const std::vector<Vector>& VertexNormals = MD.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Normal);
		return VertexNormals[VertexInstanceIDA].Equals(VertexNormals[VertexInstanceIDB], THRESH_NORMALS_ARE_SAME);
	}

	inline bool FLayoutUV::UVsMatch(uint32 a, uint32 b) const
	{
		// If current SrcChannel is out of range of the number of UVs defined by the mesh description, just return true
		const uint32 NumUVs = MD.VertexInstanceAttributes().GetAttributeIndexCount<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);
		if (SrcChannel >= NumUVs)
		{
			//ensure(false);	// not expecting it to get here
			return true;
		}

		const int VertexInstanceIDA(RemapVerts[a]);
		const int VertexInstanceIDB(RemapVerts[b]);

		const std::vector<Vector2>& VertexUVs = MD.VertexInstanceAttributes().GetAttributes<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, SrcChannel);
		return VertexUVs[VertexInstanceIDA].Equals(VertexUVs[VertexInstanceIDB], GetUVEqualityThreshold());
	}

	inline bool FLayoutUV::VertsMatch(uint32 a, uint32 b) const
	{
		return PositionsMatch(a, b) && UVsMatch(a, b);
	}

	// Signed UV area
	inline float FLayoutUV::TriangleUVArea(uint32 Tri) const
	{
		const std::vector<Vector2>& VertexUVs = MD.VertexInstanceAttributes().GetAttributes<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, SrcChannel);

		Vector2 UVs[3];
		for (int k = 0; k < 3; k++)
		{
			UVs[k] = VertexUVs[int(RemapVerts[(3 * Tri) + k])];
		}

		Vector2 EdgeUV1 = UVs[1] - UVs[0];
		Vector2 EdgeUV2 = UVs[2] - UVs[0];
		return 0.5f * (EdgeUV1.X * EdgeUV2.Y - EdgeUV1.Y * EdgeUV2.X);
	}

	inline void FLayoutUV::DisconnectChart(FMeshChart& Chart, uint32 Side)
	{
		if (Chart.Join[Side] != -1)
		{
			Charts[Chart.Join[Side]].Join[Side ^ 1] = -1;
			Chart.Join[Side] = -1;
		}
	}
}