#include "FBXWalker.h"
#include "log.h"

void FBXWalker::Initialize()
{
	iNumTabs = 0;

	m_fbxManager = FbxManager::Create();

	m_IOSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);

	m_fbxManager->SetIOSettings(m_IOSettings);

	FbxImporter* lImporter = FbxImporter::Create(m_fbxManager, "");

	const char* lFileName = "ironman2.fbx";

	if (!lImporter->Initialize(lFileName, -1, m_fbxManager->GetIOSettings()))
	{
		X_LOG("Call to FbxImporter::Initialize() failed.\n");
		X_LOG("Error returned: %s \n\n",lImporter->GetStatus().GetErrorString());
		return;
	}

	FbxScene* lScene = FbxScene::Create(m_fbxManager, "myScene");
	lImporter->Import(lScene);
	lImporter->Destroy();

	FbxNode* lRootNode = lScene->GetRootNode();
	if (lRootNode)
	{
		for (int i = 0; i < lRootNode->GetChildCount(); i++)
		{
			PrintNode(lRootNode->GetChild(i));
		}
	}
}

void FBXWalker::PrintNode(FbxNode* pNode)
{
	PrintTabs();
	const char* nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();

	X_LOG("\n %s (%f,%f,%f), (%f,%f,%f),(%f,%f,%f)",
		nodeName,
		translation[0], translation[1], translation[2],
		rotation[0], rotation[1], rotation[2],
		scaling[0], scaling[1], scaling[2]
	);

	iNumTabs++;

	for (int i = 0; i < pNode->GetNodeAttributeCount(); ++i)
	{
		PrintAttribute(pNode->GetNodeAttributeByIndex(i));
	}

	for (int j = 0; j < pNode->GetChildCount(); j++)
	{
		PrintNode(pNode->GetChild(j));
	}

	iNumTabs--;

	PrintTabs();
	X_LOG("\n");
}

void FBXWalker::PrintTabs()
{
	for (int i = 0; i < iNumTabs; i++)
		X_LOG("\t");
}

void FBXWalker::PrintAttribute(FbxNodeAttribute* pAttribute)
{
	if (!pAttribute) return;

	FbxString typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
	FbxString attrName = pAttribute->GetName();
	X_LOG("\n %s %s", typeName.Buffer(), attrName.Buffer());
	switch (pAttribute->GetAttributeType())
	{
	case FbxNodeAttribute::eMesh:
		PrintMesh((FbxMesh*)pAttribute);
		break;
	}
}

void FBXWalker::PrintMesh(FbxMesh* pMesh)
{
	int iNumPolygon = pMesh->GetPolygonCount();
	for (int i = 0; i < iNumPolygon; ++i)
	{
		for (int j = 0; j < pMesh->GetPolygonSize(i); ++j)
		{
			int iControlPointIndex = pMesh->GetPolygonVertex(i,j);
			FbxVector4* pContorlPoints = pMesh->GetControlPoints();
			FbxVector4 Point = pContorlPoints[iControlPointIndex];
			X_LOG("\n %d %d (%f %f %f)",i,j, Point[0], Point[1], Point[2]);
		}
	}
}

FbxString FBXWalker::GetAttributeTypeName(FbxNodeAttribute::EType type)
{
	switch (type)
	{
	case FbxNodeAttribute::eUnknown:return "eUnknown!";
	case FbxNodeAttribute::eNull:return "eNull!";
	case FbxNodeAttribute::eMarker:return "eMarker!";
	case FbxNodeAttribute::eSkeleton: return "skeleton";
	case FbxNodeAttribute::eMesh: return "mesh";
	case FbxNodeAttribute::eNurbs: return "nurbs";
	case FbxNodeAttribute::ePatch: return "patch";
	case FbxNodeAttribute::eCamera: return "camera";
	case FbxNodeAttribute::eCameraStereo: return "stereo";
	case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
	case FbxNodeAttribute::eLight: return "light";
	case FbxNodeAttribute::eOpticalReference: return "optical reference";
	case FbxNodeAttribute::eOpticalMarker: return "marker";
	case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
	case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
	case FbxNodeAttribute::eBoundary: return "boundary";
	case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
	case FbxNodeAttribute::eShape: return "shape";
	case FbxNodeAttribute::eLODGroup: return "lodgroup";
	case FbxNodeAttribute::eSubDiv: return "subdiv";
	default: return "unknown";
	}
}

