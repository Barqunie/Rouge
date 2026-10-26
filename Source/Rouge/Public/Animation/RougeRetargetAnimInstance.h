// Runtime IK retargeting: a mesh with its own skeleton copies the (retargeted) pose of the mesh it is attached to.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "RougeRetargetAnimInstance.generated.h"

class UIKRetargeter;

/** Runs a single Retarget Pose From Mesh node, no anim graph needed. */
USTRUCT()
struct FRougeRetargetAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FRougeRetargetAnimInstanceProxy() = default;
	FRougeRetargetAnimInstanceProxy(UAnimInstance* InAnimInstance, FAnimNode_RetargetPoseFromMesh* InRetargetNode);

	virtual FAnimNode_Base* GetCustomRootNode() override { return RetargetNode; }
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override;

	FAnimNode_RetargetPoseFromMesh* RetargetNode = nullptr;
};

/**
 * Anim instance for ACharacterBase's visual mesh: retargets the pose of the parent (hidden) mesh, which keeps the
 * character's real anim blueprint, montages and hit reacts, onto a mesh with a different skeleton.
 */
UCLASS(Transient, NotBlueprintable)
class ROUGE_API URougeRetargetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Call right after the component created this instance. */
	void Configure(UIKRetargeter* InRetargeter);

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	UPROPERTY(Transient)
	FAnimNode_RetargetPoseFromMesh RetargetNode;
};
