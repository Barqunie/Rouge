#include "Animation/RougeArmedAnimInstance.h"

#include "Animation/AnimMontage.h"
#include "Character/CharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Retargeter/IKRetargeter.h"
#include "Retargeter/IKRetargetProcessor.h"
#include "UObject/Package.h"

FRougeArmedAnimInstanceProxy::FRougeArmedAnimInstanceProxy(UAnimInstance* InAnimInstance, URougeArmedAnimInstance* InNodes)
	: FAnimInstanceProxy(InAnimInstance)
	, Nodes(InNodes)
{
}

FAnimNode_Base* FRougeArmedAnimInstanceProxy::GetCustomRootNode()
{
	return Nodes ? &Nodes->RootBlend : nullptr;
}

void FRougeArmedAnimInstanceProxy::GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes)
{
	if (!Nodes)
	{
		return;
	}
	OutNodes.Append({ &Nodes->IdleNode, &Nodes->WalkNode, &Nodes->FireNode, &Nodes->WalkFireNode,
		&Nodes->MoveBlend, &Nodes->FireMoveBlend, &Nodes->FireBlend, &Nodes->RetargetNode, &Nodes->RootBlend });
}

URougeArmedAnimInstance::URougeArmedAnimInstance()
{
	MoveBlend.A.SetLinkNode(&IdleNode);
	MoveBlend.B.SetLinkNode(&WalkNode);
	FireMoveBlend.A.SetLinkNode(&FireNode);
	FireMoveBlend.B.SetLinkNode(&WalkFireNode);
	FireBlend.A.SetLinkNode(&MoveBlend);
	FireBlend.B.SetLinkNode(&FireMoveBlend);
	RootBlend.A.SetLinkNode(&FireBlend);
	RootBlend.B.SetLinkNode(&RetargetNode);
	RetargetNode.RetargetFrom = ERetargetSourceMode::ParentSkeletalMeshComponent;
}

FAnimInstanceProxy* URougeArmedAnimInstance::CreateAnimInstanceProxy()
{
	return new FRougeArmedAnimInstanceProxy(this, this);
}

void URougeArmedAnimInstance::Configure(const FRougeArmedAnimSet& InSet, UIKRetargeter* InRetargeter)
{
	Set = InSet;
	IdleNode.SetSequence(Set.Idle);
	WalkNode.SetSequence(Set.Walk ? Set.Walk.Get() : Set.Idle.Get());
	FireNode.SetSequence(Set.Fire ? Set.Fire.Get() : Set.Idle.Get());
	WalkFireNode.SetSequence(Set.WalkFire ? Set.WalkFire.Get() : FireNode.GetSequence());
	for (FAnimNode_SequencePlayer_Standalone* Player : { &IdleNode, &WalkNode, &FireNode, &WalkFireNode })
	{
		Player->SetLoopAnimation(true);
	}

	RetargetNode.IKRetargeterAsset = InRetargeter;
	if (FIKRetargetProcessor* Processor = RetargetNode.GetRetargetProcessor())
	{
		Processor->SetNeedsInitialized();
	}
}

void URougeArmedAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ACharacterBase* Character = Cast<ACharacterBase>(GetOwningActor());
	const USkeletalMeshComponent* SourceMesh = Character ? Character->GetMesh() : nullptr;
	if (!SourceMesh)
	{
		return;
	}

	// Walk speed drives the move blend and the walk play rate (in-place clips).
	const float Speed = Character->GetVelocity().Size2D();
	const float WalkRate = Speed > 10.f ? FMath::Clamp(Speed / Set.WalkAnimSpeed, 0.6f, 1.8f) : 1.f;
	WalkNode.SetPlayRate(WalkRate);
	WalkFireNode.SetPlayRate(WalkRate);

	// What the character mesh is doing: shooting, something we have no clip for, or nothing special.
	bool bShooting = false;
	bool bOtherAction = Character->IsDead() || SourceMesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint;
	if (const UAnimInstance* SourceAnim = SourceMesh->GetAnimInstance())
	{
		for (const FAnimMontageInstance* Instance : SourceAnim->MontageInstances)
		{
			const UAnimMontage* Montage = Instance && Instance->IsActive() ? Instance->Montage.Get() : nullptr;
			if (!Montage)
			{
				continue;
			}
			if (Set.FireMontages.Contains(Montage))
			{
				bShooting = true;
			}
			// Hit reacts are dynamic montages (transient package): the armed pose just keeps going under them.
			else if (Montage->GetOutermost() != GetTransientPackage())
			{
				bOtherAction = true;
			}
		}
	}
	FireTimer = bShooting ? Set.FireHoldTime : FMath::Max(0.f, FireTimer - DeltaSeconds);

	MoveAlpha = FMath::FInterpTo(MoveAlpha, FMath::Clamp(Speed / 120.f, 0.f, 1.f), DeltaSeconds, 10.f);
	FireAlpha = FMath::FInterpTo(FireAlpha, FireTimer > 0.f ? 1.f : 0.f, DeltaSeconds, 14.f);
	RetargetAlpha = FMath::FInterpTo(RetargetAlpha, bOtherAction ? 1.f : 0.f, DeltaSeconds, 10.f);
	MoveBlend.Alpha = MoveAlpha;
	FireMoveBlend.Alpha = MoveAlpha;
	FireBlend.Alpha = FireAlpha;
	RootBlend.Alpha = RetargetAlpha;
}
