#include "Animation/RougeRetargetAnimInstance.h"

#include "Retargeter/IKRetargeter.h"
#include "Retargeter/IKRetargetProcessor.h"

FRougeRetargetAnimInstanceProxy::FRougeRetargetAnimInstanceProxy(UAnimInstance* InAnimInstance, FAnimNode_RetargetPoseFromMesh* InRetargetNode)
	: FAnimInstanceProxy(InAnimInstance)
	, RetargetNode(InRetargetNode)
{
}

void FRougeRetargetAnimInstanceProxy::GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes)
{
	OutNodes.Add(RetargetNode);
}

FAnimInstanceProxy* URougeRetargetAnimInstance::CreateAnimInstanceProxy()
{
	return new FRougeRetargetAnimInstanceProxy(this, &RetargetNode);
}

void URougeRetargetAnimInstance::Configure(UIKRetargeter* InRetargeter)
{
	RetargetNode.RetargetFrom = ERetargetSourceMode::ParentSkeletalMeshComponent;
	RetargetNode.IKRetargeterAsset = InRetargeter;
	if (FIKRetargetProcessor* Processor = RetargetNode.GetRetargetProcessor())
	{
		Processor->SetNeedsInitialized();
	}
}
