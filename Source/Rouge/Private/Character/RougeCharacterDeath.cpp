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
	DOREPLIFETIME(ARougeCharacter, SpeedBuffMultiplier);
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
		Revive(false);
		return;
	}

	if (!bTeammateReviveBlocked && IsAliveTeammateInReviveRange())
	{
		ReviveProgress = FMath::Min(1.f, ReviveProgress + DeltaTime / ReviveDuration);
		if (ReviveProgress >= 1.f)
		{
			Revive(true);
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

void ARougeCharacter::Revive(bool bByTeammate)
{
	if (!HasAuthority() || !bIsDead)
	{
		return;
	}

	bIsDead = false;
	ReviveProgress = 0.f;
	bTeammateReviveBlocked = false;
	if (bByTeammate)
	{
		LastReviveServerTime = GetServerWorldTime();
	}

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

	if (ReviveAreaMesh)
	{
		SetCircleScale(ReviveAreaMesh, ReviveRadius);
		ReviveAreaMesh->SetVisibility(bIsDead);
	}

	if (ReviveProgressMesh)
	{
		ReviveProgressMesh->SetVisibility(false);
	}

	PlaceReviveCircles();

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

void ARougeCharacter::PlaceReviveCircles()
{
	// Default: at the feet. The capsule height comes from the BP (can differ from the C++ default).
	const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;
	const FVector Location = GetActorLocation();
	float FloorZ = Location.Z - HalfHeight;

	// On a ramp or when the body stopped mid-air, use the floor right under the body.
	if (const UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ReviveCircleFloor), false, this);
		if (World->LineTraceSingleByChannel(Hit, Location, Location - FVector(0.f, 0.f, HalfHeight + 500.f), ECC_Visibility, Params))
		{
			FloorZ = Hit.ImpactPoint.Z;
		}
	}

	if (ReviveAreaMesh)
	{
		ReviveAreaMesh->SetWorldLocation(FVector(Location.X, Location.Y, FloorZ + 2.f));
	}
	if (ReviveProgressMesh)
	{
		ReviveProgressMesh->SetWorldLocation(FVector(Location.X, Location.Y, FloorZ + 3.f));
	}
}

void ARougeCharacter::SetCircleScale(UStaticMeshComponent* Circle, float Radius) const
{
	if (Circle)
	{
		// The circle is attached to the (possibly scaled, e.g. tank 1.15x) actor; undo that so it matches the real radius.
		const float ActorScale = FMath::Max(GetActorScale3D().X, KINDA_SMALL_NUMBER);
		const float XY = FMath::Max(Radius * 2.f / CylinderMeshDiameter / ActorScale, 0.01f);
		Circle->SetRelativeScale3D(FVector(XY, XY, CircleThicknessScale / ActorScale));
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
