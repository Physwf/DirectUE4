#include "MeshDescription.h"

namespace MeshAttribute
{
	const std::string Vertex::Position("Position ");
	const std::string Vertex::CornerSharpness("CornerSharpness");

	const std::string VertexInstance::TextureCoordinate("TextureCoordinate");
	const std::string VertexInstance::Normal("Normal");
	const std::string VertexInstance::Tangent("Tangent");
	const std::string VertexInstance::BinormalSign("BinormalSign");
	const std::string VertexInstance::Color("Color");

	const std::string Edge::IsHard("IsHard");
	const std::string Edge::IsUVSeam("IsUVSeam");
	const std::string Edge::CreaseSharpness("CreaseSharpness");

	const std::string Polygon::Normal("Normal");
	const std::string Polygon::Tangent("Tangent");
	const std::string Polygon::Binormal("Binormal");
	const std::string Polygon::Center("Center");

	const std::string PolygonGroup::ImportedMaterialSlotName("ImportedMaterialSlotName");
	const std::string PolygonGroup::EnableCollision("EnableCollision");
	const std::string PolygonGroup::CastShadow("CastShadow");
}
