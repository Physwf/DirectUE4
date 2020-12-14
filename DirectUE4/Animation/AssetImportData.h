#pragma once

#include "UnrealMath.h"

class UAssetImportData 
{

};

class UFbxAssetImportData : public UAssetImportData
{
public:
	FVector ImportTranslation;
	FRotator ImportRotation;
	float ImportUniformScale;
	/** Whether to convert scene from FBX scene. */
	bool bConvertScene;
	/** Whether to force the front axis to be align with X instead of -Y. */
	bool bForceFrontXAxis;
	/** Whether to convert the scene from FBX unit to UE4 unit (centimeter). */
	bool bConvertSceneUnit;
	/* Use by the reimport factory to answer CanReimport, if true only factory for scene reimport will return true */
	bool bImportAsScene;
	/* Use by the reimport factory to answer CanReimport, if true only factory for scene reimport will return true */
	//UFbxSceneImportData* FbxSceneImportDataReference;
};

class UFbxAnimSequenceImportData : public UFbxAssetImportData
{
public:
	/** If checked, meshes nested in bone hierarchies will be imported instead of being converted to bones. */
	bool bImportMeshesInBoneHierarchy;
	/** Which animation range to import. The one defined at Exported, at Animated time or define a range manually */
	//enum EFBXAnimationLengthImportType AnimationLength;
	/** Start frame when Set Range is used in Animation Length */
	int32	StartFrame_DEPRECATED;
	/** End frame when Set Range is used in Animation Length  */
	int32	EndFrame_DEPRECATED;
	/** Frame range used when Set Range is used in Animation Length */
	//FInt32Interval FrameImportRange;
	/** Enable this option to use default sample rate for the imported animation at 30 frames per second */
	bool bUseDefaultSampleRate;
	/** Name of source animation that was imported, used to reimport correct animation from the FBX file */
	std::string SourceAnimationName;
	/** Import if custom attribute as a curve within the animation */
	bool bImportCustomAttribute;
	/** Import bone transform tracks. If false, this will discard any bone transform tracks. (useful for curves only animations)*/
	bool bImportBoneTracks;
	/** Set Material Curve Type for all custom attributes that exists */
	bool bSetMaterialDriveParameterOnCustomAttribute;
	/** Set Material Curve Type for the custom attribute with the following suffixes. This doesn't matter if Set Material Curve Type is true  */
	std::vector<std::string> MaterialCurveSuffixes;
	/** When importing custom attribute as curve, remove redundant keys */
	bool bRemoveRedundantKeys;
	/** If enabled, this will delete this type of asset from the FBX */
	bool bDeleteExistingMorphTargetCurves;
	/** When importing custom attribute or morphtarget as curve, do not import if it doens't have any value other than zero. This is to avoid adding extra curves to evaluate */
	bool bDoNotImportCurveWithZero;
	/** If enabled, this will import a curve within the animation */
	bool bPreserveLocalTransform;
	/** Gets or creates fbx import data for the specified anim sequence */
	static UFbxAnimSequenceImportData* GetImportDataForAnimSequence(UAnimSequence* AnimSequence, UFbxAnimSequenceImportData* TemplateForCreation);
};