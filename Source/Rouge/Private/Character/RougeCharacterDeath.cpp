// Death / revive logic for ARougeCharacter (kept separate from RougeCharacter.cpp).

#include "Character/RougeCharacter.h"

#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "AbilitySystem/RougeGameplayTag.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Game/RougeMainGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	// Engine cylinder is 100 units wide; circles are flattened to this height.
	constexpr float CylinderMeshDiameter = 100.f;
	constexpr float CircleThicknessScale = 0.01f;
	const FName CircleColorParam(TEXT("Color"));
}

void ARougeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARougeCharacter, bIsDead);
	DOREPLIFETIME(ARougeCharacter, RespawnServerTime);
	DOREPLIFETIME(ARougeCharacter, ReviveProgress);
	DOREPLIFETIME(ARougeCharacter, bTeammateReviveBlocked);
}

float ARougeCharacter::GetServerWorldTime() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
		return World->GetTimeSeconds();
	}
	return 0.f;
}

float ARougeCharacter::GetRespawnTimeRemaining() const
{
	return bIsDead ? FMath::Max(0.f, RespawnServerTime - GetServerWorldTime()) : 0.f;
}

void ARougeCharacter::HandleHealthDepleted()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	const float Now = GetServerWorldTime();

	bIsDead = true;
	ReviveProgress = 0.f;
	RespawnServerTime = Now + RespawnDelay;
	bTeammateReviveBlocked = LastReviveServerTime >= 0.f && (Now - LastReviveServerTime) < TeammateReviveCooldown;

	GetWorldTimerManager().ClearTimer(ReviveImmunityTimer);

	if (IsValid(RougeAbilitySystemComp))
	{
		RougeAbilitySystemComp->SetLooseGameplayTagCount(RougeGameplayTags::State::Invulnerable, 0);
		RougeAbilitySystemComp->CancelAllAbilities();
	}

	ApplyDeathState();
	ForceNetUpdate();

	if (ARougeMainGameMode* GameMode = GetWorld()->GetAuthGameMode<ARougeMainGameMode>())
	{
		GameMode->NotifyPlayerDied(this);
	}
}

void ARougeCharacter::TickDeathServer(float DeltaTime)
{
	if (GetServerWorldTime() >= RespawnServerTime)
	{
		Revive();
		return;
	}

	if (!bTeammateReviveBlocked && IsAliveTeammateInReviveRange())
	{
		ReviveProgress = FMath::Min(1.f, ReviveProgress + DeltaTime / ReviveDuration);
		if (ReviveProgress >= 1.f)
		{
			Revive();
		}
	}
	// Leaving the circle pauses the revive; progress is kept until the player is revived or respawns.
}

bool ARougeCharacter::IsAliveTeammateInReviveRange() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return false;
	}

	const FVector MyLocation = GetActorLocation();
	const float RadiusSq = FMath::Square(ReviveRadius);

	for (const APlayerState* OtherPlayerState : GameState->PlayerArray)
	{
		const ARougeCharacter* Teammate = OtherPlayerState ? OtherPlayerState->GetPawn<ARougeCharacter>() : nullptr;
		if (Teammate && Teammate != this && !Teammate->IsDead() &&
			FVector::DistSquared2D(Teammate->GetActorLocation(), MyLocation) <= RadiusSq)
		{
			return true;
		}
	}

	return false;
}

void ARougeCharacter::Revive()
{
	if (!HasAuthority() || !bIsDead)
	{
		return;
	}

	bIsDead = false;
	ReviveProgress = 0.f;
	bTeammateReviveBlocked = false;
	LastReviveServerTime = GetServerWorldTime();

	if (IsValid(RougeAbilitySystemComp) && IsValid(RougeAttributes))
	{
		RougeAbilitySystemComp->SetNumericAttributeBase(
			URougeAttributeSet::GetHealthAttribute(),
			RougeAttributes->GetMaxHealth() * ReviveHealthFraction);

		if (ReviveImmunityDuration > 0.f)
		{
			RougeAbilitySystemComp->SetLooseGameplayTagCount(RougeGameplayTags::State::Invulnerable, 1);
			GetWorldTimerManager().SetTimer(ReviveImmunityTimer, this, &ThisClass::ClearReviveImmunity, ReviveImmunityDuration, false);
		}
	}

	// Removes State.Dead before the auto abilities are restarted.
	ApplyDeathState();

	if (IsValid(RougeAbilitySystemComp))
	{
		RougeAbilitySystemComp->ReactivateAutoActivatedAbilities();
	}

	ForceNetUpdate();
}

void ARougeCharacter::ClearReviveImmunity()
{
	if (IsValid(RougeAbilitySystemComp))
	{
		RougeAbilitySystemComp->SetLooseGameplayTagCount(RougeGameplayTags::State::Invulnerable, 0);
	}
}

void ARougeCharacter::OnRep_IsDead()
{
	ApplyDeathState();
}

void ARougeCharacter::ApplyDeathState()
{
	// Mirrored on clients so local ability activation checks see State.Dead too.
	if (IsValid(RougeAbilitySystemComp))
	{
		RougeAbilitySystemComp->SetLooseGameplayTagCount(RougeGameplayTags::State::Dead, bIsDead ? 1 : 0);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (bIsDead)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		else
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}

	// Dead bodies don't block teammates or enemies.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, bIsDead ? ECR_Ignore : ECR_Block);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (bIsDead && DeathAnimation)
		{
			MeshComp->PlayAnimation(DeathAnimation, false);
		}
		else if (!bIsDead && MeshComp->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
		{
			MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}
	}

	// Put the circles just above the floor. The capsule height comes from the BP (can differ from the C++ default).
	const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;

	if (ReviveAreaMesh)
	{
		ReviveAreaMesh->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight + 2.f));
		SetCircleScale(ReviveAreaMesh, ReviveRadius);
		ReviveAreaMesh->SetVisibility(bIsDead);
	}

	if (ReviveProgressMesh)
	{
		ReviveProgressMesh->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight + 3.f));
		ReviveProgressMesh->SetVisibility(false);
	}

	// Dead player's own screen goes mostly black & white (local camera only).
	if (FollowCamera && IsLocallyControlled())
	{
		FPostProcessSettings& PostProcess = FollowCamera->PostProcessSettings;
		PostProcess.bOverride_ColorSaturation = bIsDead;
		PostProcess.ColorSaturation = FVector4(DeadScreenSaturation, DeadScreenSaturation, DeadScreenSaturation, 1.f);
		if (bIsDead)
		{
			FollowCamera->PostProcessBlendWeight = 1.f;
		}
	}

	OnDeathStateChanged(bIsDead);

	if (bIsDead)
	{
		UpdateDeathPresentation();
	}
}

void ARougeCharacter::UpdateDeathPresentation()
{
	if (ReviveAreaMID)
	{
		ReviveAreaMID->SetVectorParameterValue(CircleColorParam, bTeammateReviveBlocked ? ReviveBlockedColor : ReviveAreaColor);
	}

	if (ReviveProgressMesh)
	{
		const bool bShowProgress = ReviveProgress > KINDA_SMALL_NUMBER;
		ReviveProgressMesh->SetVisibility(bShowProgress);
		if (bShowProgress)
		{
			SetCircleScale(ReviveProgressMesh, ReviveRadius * ReviveProgress);
		}
	}

	OnDeathTick(GetRespawnTimeRemaining(), ReviveProgress, bTeammateReviveBlocked);
}

void ARougeCharacter::SetCircleScale(UStaticMeshComponent* Circle, float Radius) const
{
	if (Circle)
	{
		const float XY = FMath::Max(Radius * 2.f / CylinderMeshDiameter, 0.01f);
		Circle->SetRelativeScale3D(FVector(XY, XY, CircleThicknessScale));
	}
}

void ARougeCharacter::InitReviveCircleMaterials()
{
	if (ReviveAreaMesh)
	{
		if (ReviveCircleMaterial)
		{
			ReviveAreaMesh->SetMaterial(0, ReviveCircleMaterial);
		}
		ReviveAreaMID = ReviveAreaMesh->CreateDynamicMaterialInstance(0);
		if (ReviveAreaMID)
		{
			ReviveAreaMID->SetVectorParameterValue(CircleColorParam, ReviveAreaColor);
		}
	}

	if (ReviveProgressMesh)
	{
		if (ReviveCircleMaterial)
		{
			ReviveProgressMesh->SetMaterial(0, ReviveCircleMaterial);
		}
		ReviveProgressMID = ReviveProgressMesh->CreateDynamicMaterialInstance(0);
		if (ReviveProgressMID)
		{
			ReviveProgressMID->SetVectorParameterValue(CircleColorParam, ReviveProgressColor);
		}
	}
}

void ARougeCharacter::UpdateShieldBubble(float ShieldValue)
{
	if (!ShieldBubbleMesh)
	{
		return;
	}

	const bool bShow = ShieldValue > 0.f;
	if (bShow && !ShieldBubbleMID)
	{
		if (ReviveCircleMaterial)
		{
			ShieldBubbleMesh->SetMaterial(0, ReviveCircleMaterial);
		}
		ShieldBubbleMID = ShieldBubbleMesh->CreateDynamicMaterialInstance(0);
		if (ShieldBubbleMID)
		{
			ShieldBubbleMID->SetVectorParameterValue(CircleColorParam, ShieldBubbleColor);
		}
	}

	ShieldBubbleMesh->SetVisibility(bShow);
}
