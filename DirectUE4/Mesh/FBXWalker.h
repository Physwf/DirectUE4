#pragma once

#include "fbxsdk.h"

class FBXWalker
{
public:
	FBXWalker() {}
	~FBXWalker() {}

	void Initialize();

	void PrintNode(FbxNode* pNode);
	void PrintTabs();
	void PrintAttribute(FbxNodeAttribute* pAttribute);
	void PrintMesh(FbxMesh* pMesh);
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType type);
private:
	FbxManager*			m_fbxManager;
	FbxIOSettings*		m_IOSettings;
	FbxScene*			m_fbxScence;

	int iNumTabs;
};