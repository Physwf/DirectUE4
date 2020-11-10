#pragma once

#include <map>
#include <unordered_map>

class MeshDescription;

class MeshDescriptionOperations
{
public:
	enum ETangentOptions
	{
		None = 0,
		BlendOverlappingNormals = 0x1,
		IgnoreDegenerateTriangles = 0x2,
		UseMikkTSpace = 0x4,
	};

	enum class ELightmapUVVersion : int
	{
		BitByBit = 0,
		Segments = 1,
		SmallChartPacking = 2,
		Latest = SmallChartPacking
	};

	static void CreatePolygonNTB(MeshDescription& MD, float ComparisonThreshold);

	static void CreateMikktTangents(MeshDescription& MD, ETangentOptions TangentOptions);

	static void FindOverlappingCorners(std::multimap<int, int>& OverlappingCorners, const MeshDescription& MD, float ComparisonThreshold);

	static void CreateLightMapUVLayout(MeshDescription& MD,
		int SrcLightmapIndex,
		int DstLightmapIndex,
		int MinLightmapResolution,
		ELightmapUVVersion LightmapUVVersion,
		const std::multimap<int, int>& OverlappingCorners);
};