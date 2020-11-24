#include "DeferredShading.h"
#include "LightSceneInfo.h"

void SceneRenderer::InitDynamicShadows()
{
	for (auto LightIt = Scene->Lights.begin();LightIt!= Scene->Lights.end();++LightIt )
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = **LightIt;
		FLightSceneInfo* LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		const bool bAllowStaticLighting = true;
		const bool bPointLightShadow = LightSceneInfoCompact.LightType == LightType_Point || LightSceneInfoCompact.LightType == LightType_Rect;
		/*
		CreateWholeSceneProjectedShadow(LightSceneInfo, NumPointShadowCachesUpdatedThisFrame, NumSpotShadowCachesUpdatedThisFrame);

		if ((!LightSceneInfo->Proxy->HasStaticLighting() && LightSceneInfoCompact.bCastDynamicShadow))
		{
			AddViewDependentWholeSceneShadowsForView(ViewDependentWholeSceneShadows, ViewDependentWholeSceneShadowsThatNeedCulling, VisibleLightInfo, *LightSceneInfo);

			// Look for individual primitives with a dynamic shadow.
			for (FLightPrimitiveInteraction* Interaction = LightSceneInfo->DynamicInteractionOftenMovingPrimitiveList;
				Interaction;
				Interaction = Interaction->GetNextPrimitive()
				)
			{
				SetupInteractionShadows(RHICmdList, Interaction, VisibleLightInfo, bStaticSceneOnly, ViewDependentWholeSceneShadows, PreShadows);
			}

			for (FLightPrimitiveInteraction* Interaction = LightSceneInfo->DynamicInteractionStaticPrimitiveList;
				Interaction;
				Interaction = Interaction->GetNextPrimitive()
				)
			{
				SetupInteractionShadows(RHICmdList, Interaction, VisibleLightInfo, bStaticSceneOnly, ViewDependentWholeSceneShadows, PreShadows);
			}
		}
		*/
	}

	AllocateShadowDepthTargets();
}

void SceneRenderer::AllocateShadowDepthTargets()
{

}