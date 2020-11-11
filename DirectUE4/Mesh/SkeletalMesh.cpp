#include "SkeletalMesh.h"
#include "log.h"

SkeletalMesh::SkeletalMesh()
{
	ImportedModel = std::make_unique<SkeletalMeshModel>();

}
