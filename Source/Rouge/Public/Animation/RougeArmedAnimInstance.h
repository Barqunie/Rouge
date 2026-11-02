// A weapon-holding look with its own clips (idle / walk / fire), falling back to retargeting for everything else.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "Animation/RougeArmedAnimSet.h"
#include "RougeArmedAnimInstance.generated.h"

class URougeArmedAnimInstance;
class UIKRetargeter;

USTRUCT()
struct FRougeArmedAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FRougeArmedAnimInstanceProxy() = default;
	FRougeArmedAnimInstanceProxy(UAnimInstance* InAnimInstance, URougeArmedAnimInstance* InNodes);

	virtual FAnimNode_Base* GetCustomRootNode() override;
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override;

	URougeArmedAnimInstance* Nodes = nullptr;
};

/**
 * Anim instance for ACharacterBase's visual mesh when an armed anim set is given: idle <-> walk by speed, fire /
 * walk-fire while a fire montage plays on the (hidden) character mesh. Death, revive and other montages (skills)
 * blend to the pose retargeted from the character mesh, so those keep working without clips of their own.
 * Built from native nodes, no anim graph.
 */
UCLASS(Transient, NotBlueprintable)
class ROUGE_API URougeArmedAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	URougeArmedAnimInstance();

	/** Call right after the component created this instance. */
	void Configure(const FRougeArmedAnimSet& InSet, UIKRetargeter* InRetargeter);

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	friend struct FRougeArmedAnimInstanceProxy;

	UPROPERTY(Transient)
	FAnimNode_SequencePlayer_Standalone IdleNode;

	UPROPERTY(Transient)
	FAnimNode_SequencePlayer_Standalone WalkNode;

	UPROPERTY(Transient)
	FAnimNode_SequencePlayer_Standalone FireNode;

	UPROPERTY(Transient)
	FAnimNode_SequencePlayer_Standalone WalkFireNode;

	/** Idle (A) / Walk (B). */
	UPROPERTY(Transient)
	FAnimNode_TwoWayBlend MoveBlend;

	/** Fire (A) / WalkFire (B). */
	UPROPERTY(Transient)
	FAnimNode_TwoWayBlend FireMoveBlend;

	/** Holding (A) / shooting (B). */
	UPROPERTY(Transient)
	FAnimNode_TwoWayBlend FireBlend;

	UPROPERTY(Transient)
	FAnimNode_RetargetPoseFromMesh RetargetNode;

	/** Own clips (A) / retargeted character mesh (B). */
	UPROPERTY(Transient)
	FAnimNode_TwoWayBlend RootBlend;

	UPROPERTY(Transient)
	FRougeArmedAnimSet Set;

	float MoveAlpha = 0.f;
	float FireAlpha = 0.f;
	float RetargetAlpha = 0.f;
	float FireTimer = 0.f;
};
