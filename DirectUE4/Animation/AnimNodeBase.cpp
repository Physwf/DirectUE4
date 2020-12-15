#include "AnimNodeBase.h"
#include "AnimInstanceProxy.h"

FAnimationBaseContext::FAnimationBaseContext(FAnimInstanceProxy* InAnimInstanceProxy)
	: AnimInstanceProxy(InAnimInstanceProxy)
{

}

FAnimationBaseContext::FAnimationBaseContext(const FAnimationBaseContext& InContext)
{
	AnimInstanceProxy = InContext.AnimInstanceProxy;
}

void FAnimNode_Base::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{

}

void FAnimNode_Base::Evaluate_AnyThread(FPoseContext& Output)
{
	//Evaluate(Output);
}

void FAnimNode_Base::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	//Update(Context);
}

void FPoseContext::Initialize(FAnimInstanceProxy* InAnimInstanceProxy)
{
	const FBoneContainer& RequiredBone = AnimInstanceProxy->GetRequiredBones();
	Pose.SetBoneContainer(&RequiredBone);
	Curve.InitFrom(RequiredBone);
}
