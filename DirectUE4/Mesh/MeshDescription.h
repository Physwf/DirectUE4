#pragma once

#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <tuple>
#include <set>
#include <algorithm>

#include "UnrealMath.h"

namespace MeshAttribute
{
	namespace Vertex
	{
		extern const std::string Position;
		extern const std::string CornerSharpness;
	}

	namespace VertexInstance
	{
		extern const std::string TextureCoordinate;
		extern const std::string Normal;
		extern const std::string Tangent;
		extern const std::string BinormalSign;
		extern const std::string Color;
	}

	namespace Edge
	{
		extern const std::string IsHard;
		extern const std::string IsUVSeam;
		extern const std::string CreaseSharpness;
	}

	namespace Polygon
	{
		extern const std::string Normal;
		extern const std::string Tangent;
		extern const std::string Binormal;
		extern const std::string Center;
	}

	namespace PolygonGroup
	{
		extern const std::string ImportedMaterialSlotName;
		extern const std::string EnableCollision;
		extern const std::string CastShadow;
	}
}

struct MeshVertex
{
	MeshVertex() {}

	std::vector<int> VertexInstanceIDs;
	std::set<int> ConnectedEdgeIDs;
};

struct MeshVertexInstance
{
	MeshVertexInstance() :VertexID(-1) {}

	int VertexID;
	std::vector<int> ConnectedPolygons;
};

struct MeshEdge
{
	MeshEdge()
	{
		VertexIDs[0] = -1;
		VertexIDs[1] = -1;
	}

	int VertexIDs[2];
	std::vector<int> ConnectedPolygons;
};

struct MeshPolygonContour
{
	std::vector<int> VertexInstanceIDs;
};

struct MeshTriangle
{
	int VertexInstanceID0;
	int VertexInstanceID1;
	int VertexInstanceID2;

	inline int GetVertexInstanceID(int Index) const
	{
		return reinterpret_cast<const int*>(this)[Index];
	}
	void SetVertexInstanceID(int Index, int ID)
	{
		reinterpret_cast<int*>(this)[Index] = ID;
	}
};

struct MeshPolygon
{
	MeshPolygon() :PolygonGroupID(-1) {}

	MeshPolygonContour PerimeterContour;

	std::vector<MeshPolygonContour> HoleContours;

	std::vector<MeshTriangle> Triangles;

	int PolygonGroupID;
};

struct MeshPolygonGroup
{
	MeshPolygonGroup() {}

	std::vector<int> Polygons;
};


template <typename ElementType>
class TMeshElementArray
{
public:

	inline void Reset(const int32 Elements = 0)
	{
		Container.empty();
		Container.reserve(Elements);
	}

	inline void Reserve(const int32 Elements) { /*Container.Reserve(Elements);*/ }

	inline int Add()
	{
		Container.insert({ MaxID, ElementType() });
		return MaxID++;
	}

	inline int Add(const typename std::remove_reference<ElementType>::type& Element)
	{
		Container.insert(MaxID, Element);
		return MaxID++;
	}

	inline int Add(ElementType&& Element)
	{
		Container.Add(MaxID, std::forward<ElementType>(Element));
		return MaxID++;
	}

	inline ElementType& Insert(int ID)
	{
		Container.insert(ID, ElementType());
		return Container[ID];
	}

	inline ElementType& Insert(int ID, const typename std::remove_reference<ElementType>::type& Element)
	{
		Container.insert(ID, Element);
		return Container[ID];
	}

	inline ElementType& Insert(int ID, ElementType&& Element)
	{
		Container.insert(ID, std::forward<ElementType>(Element));
		return Container[ID];
	}

	inline void Remove(int ID)
	{
		Container.RemoveAt(ID);
	}

	inline ElementType& operator[](const int ID)
	{
		return Container[ID];
	}

	inline const ElementType& operator[](int ID) const
	{
		return Container.at(ID);
	}

	inline int Num() const { return (int)Container.size(); }

	inline int GetArraySize() const { return MaxID; }

	inline int GetFirstValidID() const
	{
		return MaxID > 0 ? Container.begin()->first : -1;
	}

	inline bool IsValid(const int ID) const
	{
		return ID >= 0 && ID < MaxID && Container.find(ID) != Container.end();
	}

	std::vector<int> GetElementIDs() const
	{ 
		std::vector<int> Keys;
		for (auto kv : Container)
		{
			Keys.push_back(kv.first);
		}
		std::sort(Keys.begin(), Keys.end());
		return Keys;
	}
private:
	int MaxID{ 0 };
	std::unordered_map<int, ElementType> Container;
};

using AttributeTypes = std::tuple
<
	Vector4,
	FVector,
	Vector2,
	float,
	int,
	bool,
	std::string
>;

template <class T, class Tuple>
struct TTupleIndex;

template <class T, class... Types>
struct TTupleIndex<T, std::tuple<T, Types...>> {
	static const std::size_t Value = 0;
};

template <class T, class U, class... Types>
struct TTupleIndex<T, std::tuple<U, Types...>> {
	static const std::size_t Value = 1U + TTupleIndex<T, std::tuple<Types...>>::Value;
};

class AttributeSet
{
	std::tuple
		<
		std::map<std::string, std::vector<std::vector<Vector4>>>,
		std::map<std::string, std::vector<std::vector<FVector>>>,
		std::map<std::string, std::vector<std::vector<Vector2>>>,
		std::map<std::string, std::vector<std::vector<float>>>,
		std::map<std::string, std::vector<std::vector<int>>>,
		std::map<std::string, std::vector<std::vector<bool>>>,
		std::map<std::string, std::vector<std::vector<std::string>>>
		>
		Containers;
public:
	template <typename AttributeType>
	void RegisterAttribute(const std::string& AttriName, const int NumOfIndices, const AttributeType& DefaultValue = AttributeType())
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		if (Map.find(AttriName) == Map.end())
		{
			std::vector<std::vector<AttributeType>> Attributes;
			Attributes.resize(NumOfIndices, std::vector<AttributeType>());
			Map.emplace(AttriName, Attributes);
		}
	}

	template <typename AttributeType>
	void UnregisterAttribute(const std::string& AttriName)
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		Map.erase(AttriName);
	}

	template <typename AttributeType>
	bool HasAttribute(const std::string& AttriName)
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		return Map.find(AttriName) != Map.end();
	}

	template <typename AttributeType>
	std::vector<AttributeType>& GetAttributes(const std::string& AttributeName, const int AttributeIndex = 0)
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		return Map.at(AttributeName)[AttributeIndex];
	}

	template <typename AttributeType>
	const std::vector<AttributeType>& GetAttributes(const std::string& AttributeName, const int AttributeIndex = 0) const
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		return Map.at(AttributeName)[AttributeIndex];
	}

	template <typename AttributeType>
	std::vector<std::vector<AttributeType>>& GetAttributesSet(const std::string& AttributeName)
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		return Map.at(AttributeName);
	}

	template <typename AttributeType>
	const std::vector<std::vector<AttributeType>>& GetAttributesSet(const std::string& AttributeName) const
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		return Map.at(AttributeName);
	}

	template <typename AttributeType>
	int GetAttributeIndexCount(const std::string& AttributeName) const
	{
		//return Container.template Get<TTupleIndex<AttributeType, AttributeTypes>::Value>().GetAttributeIndexCount(AttributeName);
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		return (int)Map.at(AttributeName).size();
	}

	template <typename AttributeType>
	void SetAttributeIndexCount(const std::string& AttributeName, int NumIndices)
	{

	}

	template <typename AttributeType>
	AttributeType GetAttribute(const int ElementID, const std::string& AttributeName, const int32 AttributeIndex = 0) const
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		return Map.at(AttributeName)[AttributeIndex][ElementID];
	}

	template <typename AttributeType>
	void SetAttribute(const int ElementID, const std::string& AttributeName, const int32 AttributeIndex, const AttributeType& AttributeValue)
	{
		auto& Map = std::get<TTupleIndex<AttributeType, AttributeTypes>::Value>(Containers);
		Map.at(AttributeName)[AttributeIndex][ElementID] = AttributeValue;
	}

	void Insert(const int ElementID)
	{
		for (auto& Pair : std::get<0>(Containers))
		{
			for (std::vector<Vector4>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes.resize(ElementID + 1, Vector4());
				}
			}
		}
		for (auto& Pair : std::get<1>(Containers))
		{
			for (std::vector<FVector>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes.resize(ElementID + 1, FVector());
				}
			}
		}
		for (auto& Pair : std::get<2>(Containers))
		{
			for (std::vector<Vector2>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes.resize(ElementID + 1, Vector2());
				}
			}
		}
		for (auto& Pair : std::get<3>(Containers))
		{
			for (std::vector<float>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes.resize(ElementID + 1, 0.0f);
				}
			}
		}
		for (auto& Pair : std::get<4>(Containers))
		{
			for (std::vector<int>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes.resize(ElementID + 1, 0);
				}
			}
		}
		for (auto& Pair : std::get<5>(Containers))
		{
			for (std::vector<bool>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes.resize(ElementID + 1, false);
				}
			}
		}
		for (auto& Pair : std::get<6>(Containers))
		{
			for (std::vector<std::string>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes.resize(ElementID + 1, std::string());
				}
			}
		}
	}

	void Remove(const int ElementID)
	{
		for (auto& Pair : std::get<0>(Containers))
		{
			for (std::vector<Vector4>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes[ElementID] = Vector4();
				}
			}
		}
		for (auto& Pair : std::get<1>(Containers))
		{
			for (std::vector<FVector>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes[ElementID] = FVector();
				}
			}
		}
		for (auto& Pair : std::get<2>(Containers))
		{
			for (std::vector<Vector2>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes[ElementID] = Vector2();
				}
			}
		}
		for (auto& Pair : std::get<3>(Containers))
		{
			for (std::vector<float>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes[ElementID] = 0.0f;
				}
			}
		}
		for (auto& Pair : std::get<4>(Containers))
		{
			for (std::vector<int>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes[ElementID] = 0;
				}
			}
		}
		for (auto& Pair : std::get<5>(Containers))
		{
			for (std::vector<bool>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes[ElementID] = false;
				}
			}
		}
		for (auto& Pair : std::get<6>(Containers))
		{
			for (std::vector<std::string>& Attributes : Pair.second)
			{
				if (ElementID >= (int)Attributes.size())
				{
					Attributes[ElementID] = std::string();
				}
			}
		}
	}

	void Initialize(const int NumElements)
	{
		for (auto& Pair : std::get<0>(Containers))
		{
			for (std::vector<Vector4>& Attributes : Pair.second)
			{
				Attributes.resize(NumElements);
			}
		}
		for (auto& Pair : std::get<1>(Containers))
		{
			for (std::vector<FVector>& Attributes : Pair.second)
			{
				Attributes.resize(NumElements);
			}
		}
		for (auto& Pair : std::get<2>(Containers))
		{
			for (std::vector<Vector2>& Attributes : Pair.second)
			{
				Attributes.resize(NumElements);
			}
		}
		for (auto& Pair : std::get<3>(Containers))
		{
			for (std::vector<float>& Attributes : Pair.second)
			{
				Attributes.resize(NumElements);
			}
		}
		for (auto& Pair : std::get<4>(Containers))
		{
			for (std::vector<int>& Attributes : Pair.second)
			{
				Attributes.resize(NumElements);
			}
		}
		for (auto& Pair : std::get<5>(Containers))
		{
			for (std::vector<bool>& Attributes : Pair.second)
			{
				Attributes.resize(NumElements);
			}
		}
		for (auto& Pair : std::get<6>(Containers))
		{
			for (std::vector<std::string>& Attributes : Pair.second)
			{
				Attributes.resize(NumElements);
			}
		}

		Insert(NumElements - 1);
	}
};

class MeshDescription
{
public:

	TMeshElementArray<MeshVertex> & Vertices() { return VertexArray; }
	const TMeshElementArray<MeshVertex>& Vertices() const { return VertexArray; }

	MeshVertex& GetVertex(const int VertexID) { return VertexArray[VertexID]; }
	const MeshVertex& GetVertex(const int VertexID) const { return VertexArray[VertexID]; }

	TMeshElementArray<MeshVertexInstance>& VertexInstances() { return VertexInstanceArray; }
	const TMeshElementArray<MeshVertexInstance>& VertexInstances() const { return VertexInstanceArray; }

	MeshVertexInstance& GetVertexInstance(const int VertexInstanceID) { return VertexInstanceArray[VertexInstanceID]; }
	const MeshVertexInstance& GetVertexInstance(const int VertexInstanceID) const { return VertexInstanceArray[VertexInstanceID]; }

	TMeshElementArray<MeshEdge>& Edges() { return EdgeArray; }
	const TMeshElementArray<MeshEdge>& Edges() const { return EdgeArray; }

	MeshEdge& GetEdge(const int EdgeID) { return EdgeArray[EdgeID]; }
	const MeshEdge& GetEdge(const int EdgeID) const { return EdgeArray[EdgeID]; }

	TMeshElementArray<MeshPolygon>& Polygons() { return PolygonArray; }
	const TMeshElementArray<MeshPolygon>& Polygons() const { return PolygonArray; }

	MeshPolygon& GetPolygon(const int PolygonID) { return PolygonArray[PolygonID]; }
	const MeshPolygon& GetPolygon(const int PolygonID) const { return PolygonArray[PolygonID]; }

	TMeshElementArray<MeshPolygonGroup>& PolygonGroups() { return PolygonGroupArray; }
	const TMeshElementArray<MeshPolygonGroup>& PolygonGroups() const { return PolygonGroupArray; }

	MeshPolygonGroup& GetPolygonGroup(const int PolygonGroupID) { return PolygonGroupArray[PolygonGroupID]; }
	const MeshPolygonGroup& GetPolygonGroup(const int PolygonGroupID) const { return PolygonGroupArray[PolygonGroupID]; }

	AttributeSet& VertexAttributes() { return VertexAttributesSet; }
	const AttributeSet& VertexAttributes() const { return VertexAttributesSet; }

	AttributeSet& VertexInstanceAttributes() { return VertexInstanceAttributesSet; }
	const AttributeSet& VertexInstanceAttributes() const { return VertexInstanceAttributesSet; }

	AttributeSet& EdgeAttributes() { return EdgeAttributesSet; }
	const AttributeSet& EdgeAttributes() const { return EdgeAttributesSet; }

	AttributeSet& PolygonAttributes() { return PolygonAttributesSet; }
	const AttributeSet& PolygonAttributes() const { return PolygonAttributesSet; }

	AttributeSet& PolygonGroupAttributes() { return PolygonGroupAttributesSet; }
	const AttributeSet& PolygonGroupAttributes() const { return PolygonGroupAttributesSet; }

	const std::set<int32>& GetVertexConnectedEdges(const int32 VertexID) const
	{
		return VertexArray[VertexID].ConnectedEdgeIDs;
	}
	const std::vector<int>& GetEdgeConnectedPolygons(const int32 EdgeID) const
	{
		return EdgeArray[EdgeID].ConnectedPolygons;
	}

private:
	void CreateVertex_Internal(int VertexID)
	{
		VertexAttributesSet.Insert(VertexID);
	}
public:
	int CreateVertex()
	{
		int VertexID = VertexArray.Add();
		CreateVertex_Internal(VertexID);
		return VertexID;
	}
private:
	void CreateVertexInstance_Internal(const int VertexInstanceID, const int VertexID)
	{
		VertexInstanceArray[VertexInstanceID].VertexID = VertexID;
		VertexArray[VertexID].VertexInstanceIDs.push_back(VertexInstanceID);
		VertexInstanceAttributesSet.Insert(VertexInstanceID);
	}
public:
	int CreateVertexInstance(const int VertexID)
	{
		const int VertexInstanceID = VertexInstanceArray.Add();
		CreateVertexInstance_Internal(VertexInstanceID, VertexID);
		return VertexInstanceID;
	}
private:
	void CreateEdge_Interal(const int EdgeID, const int VertexID0, const int VertexID1, std::vector<int> ConnectedPolygons)
	{
		MeshEdge& Edge = EdgeArray[EdgeID];
		Edge.VertexIDs[0] = VertexID0;
		Edge.VertexIDs[1] = VertexID1;
		Edge.ConnectedPolygons = ConnectedPolygons;
		VertexArray[VertexID0].ConnectedEdgeIDs.insert(EdgeID);
		VertexArray[VertexID1].ConnectedEdgeIDs.insert(EdgeID);
		EdgeAttributesSet.Insert(EdgeID);
	}
public:
	int CreateEdge(const int VertexID0, const int VertexID1, std::vector<int> ConnectedPolygons = std::vector<int>())
	{
		const int EdgeID = EdgeArray.Add();
		CreateEdge_Interal(EdgeID, VertexID0, VertexID1, ConnectedPolygons);
		return EdgeID;
	}
	struct ContourPoint
	{
		int VertexInstanceID;
		int EdgeID;
	};
private:
	void CreatePolygonContour_Internal(const int PolygonID, const std::vector<ContourPoint>& ContourPoints, MeshPolygonContour& Contour)
	{
		Contour.VertexInstanceIDs.clear();
		for (const ContourPoint CP : ContourPoints)
		{
			const int VertexInstanceID = CP.VertexInstanceID;
			const int EdgeID = CP.EdgeID;

			Contour.VertexInstanceIDs.push_back(VertexInstanceID);
			//check(!VertexInstanceArray[VertexInstanceID].ConnectedPolygons.Contains(PolygonID));
			VertexInstanceArray[VertexInstanceID].ConnectedPolygons.push_back(PolygonID);

			//check(!EdgeArray[EdgeID].ConnectedPolygons.Contains(PolygonID));
			EdgeArray[EdgeID].ConnectedPolygons.push_back(PolygonID);
		}
	}

	void CreatePolygon_Internal(const int PolygonID, const int PolygonGroupID, const std::vector<ContourPoint>& Perimeter, const std::vector<std::vector<ContourPoint>>& Holes)
	{
		MeshPolygon& Polygon = PolygonArray[PolygonID];
		CreatePolygonContour_Internal(PolygonID, Perimeter, Polygon.PerimeterContour);
		for (const std::vector<ContourPoint>& Hole : Holes)
		{
			Polygon.HoleContours.emplace(Polygon.HoleContours.end());
			CreatePolygonContour_Internal(PolygonID, Hole, Polygon.HoleContours.back());
		}
		Polygon.PolygonGroupID = PolygonGroupID;
		PolygonGroupArray[PolygonGroupID].Polygons.push_back(PolygonID);

		PolygonAttributesSet.Insert(PolygonID);
	}
public:
	int CreatePolygon(const int PolygonGroupID, const std::vector<ContourPoint>& Perimeter, const std::vector<std::vector<ContourPoint>>& Holes = std::vector<std::vector<ContourPoint>>())
	{
		const int PolygonID = PolygonArray.Add();
		CreatePolygon_Internal(PolygonID, PolygonGroupID, Perimeter, Holes);
		return PolygonID;
	}
private:
	void CreatePolygonGroup_Internal(const int PolygonGroupID)
	{
		PolygonGroupAttributesSet.Insert(PolygonGroupID);
	}
public:
	int CreatePolygonGroup()
	{
		const int PolygonGroupID = PolygonGroupArray.Add();
		CreatePolygonGroup_Internal(PolygonGroupID);
		return PolygonGroupID;
	}
public:
	int GetVertexPairEdge(const int VertexID0, const int VertexID1) const
	{
		for (const int VertexConnectedEdgeID : VertexArray[VertexID0].ConnectedEdgeIDs)
		{
			const int EdgeVertexID0 = EdgeArray[VertexConnectedEdgeID].VertexIDs[0];
			const int EdgeVertexID1 = EdgeArray[VertexConnectedEdgeID].VertexIDs[1];
			if ((EdgeVertexID0 == VertexID0 && EdgeVertexID1 == VertexID1) || (EdgeVertexID0 == VertexID1 && EdgeVertexID1 == VertexID0))
			{
				return VertexConnectedEdgeID;
			}
		}

		return -1;
	}

	std::vector<MeshTriangle>& GetPolygonTriangles(const int PolygonID)
	{
		return PolygonArray[PolygonID].Triangles;
	}

	const std::vector<MeshTriangle>& GetPolygonTriangles(const int PolygonID) const
	{
		return PolygonArray[PolygonID].Triangles;
	}

	const std::vector<int>& GetPolygonPerimeterVertexInstances(const int PolygonID) const
	{
		return PolygonArray[PolygonID].PerimeterContour.VertexInstanceIDs;
	}

	int GetVertexInstanceVertex(const int VertexInstanceID) const
	{
		return VertexInstanceArray[VertexInstanceID].VertexID;
	}
	int GetPolygonPolygonGroup(const int PolygonID) const
	{
		return PolygonArray[PolygonID].PolygonGroupID;
	}
	void ComputePolygonTriangulation(const int PolygonID, std::vector<MeshTriangle>& OutTriangles)
	{
		OutTriangles.clear();

		// @todo mesheditor holes: Does not support triangles with holes yet!
		// @todo mesheditor: Perhaps should always attempt to triangulate by splitting polygons along the shortest edge, for better determinism.

		//	const FMeshPolygon& Polygon = GetPolygon( PolygonID );
		const std::vector<int>& PolygonVertexInstanceIDs = GetPolygonPerimeterVertexInstances(PolygonID);

		// Polygon must have at least three vertices/edges
		const int PolygonVertexCount = (int)PolygonVertexInstanceIDs.size();
		//check(PolygonVertexCount >= 3);

		// If perimeter has 3 vertices, just copy content of perimeter out 
		if (PolygonVertexCount == 3)
		{
			OutTriangles.emplace(OutTriangles.end());
			MeshTriangle& Triangle = OutTriangles.back();

			Triangle.SetVertexInstanceID(0, PolygonVertexInstanceIDs[0]);
			Triangle.SetVertexInstanceID(1, PolygonVertexInstanceIDs[1]);
			Triangle.SetVertexInstanceID(2, PolygonVertexInstanceIDs[2]);

			return;
		}
	}
private:
	TMeshElementArray<MeshVertex> VertexArray;
	TMeshElementArray<MeshVertexInstance> VertexInstanceArray;
	TMeshElementArray<MeshEdge> EdgeArray;
	TMeshElementArray<MeshPolygon> PolygonArray;
	TMeshElementArray<MeshPolygonGroup> PolygonGroupArray;

	AttributeSet VertexAttributesSet;
	AttributeSet VertexInstanceAttributesSet;
	AttributeSet EdgeAttributesSet;
	AttributeSet PolygonAttributesSet;
	AttributeSet PolygonGroupAttributesSet;
};
