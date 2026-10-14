// Aim mode (mouse / auto aim) for ARougeCharacter. Runs on the owning client only.

#include "Character/RougeCharacter.h"

#include "Character/EnemyBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Game/RougeGameState.h"
#include "GameFramework/CharacterMovementComponent.h"

void ARougeCharacter::ToggleAimMode()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	bAutoAim = !bAutoAim;
	AutoAimTarget = nullptr;
	NextAutoAimSearchTime = 0.f;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, bAutoAim ? TEXT("Auto Aim: ON") : TEXT("Auto Aim: OFF (mouse)"));
	}

	OnAimModeChanged(bAutoAim);
}

AActor* ARougeCharacter::FindAutoAimTarget() const
{
	const FVector MyLocation = GetActorLocation();
	const float RangeSq = FMath::Square(AutoAimRange);

	AActor* Closest = nullptr;
	float ClosestDistSq = RangeSq;

	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		AEnemyBase* Enemy = *It;
		if (!IsValid(Enemy) || Enemy->IsDead())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(Enemy->GetActorLocation(), MyLocation);
		if (DistSq <= ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Enemy;
		}
	}

	return Closest;
}

bool ARougeCharacter::GetAutoAimPoint(FVector& OutPoint)
{
	const float Now = GetWorld()->GetTimeSeconds();

	AActor* Target = AutoAimTarget.Get();
	const ACharacterBase* TargetCharacter = Cast<ACharacterBase>(Target);
	const bool bTargetLost = !IsValid(Target) ||
		(TargetCharacter && TargetCharacter->IsDead()) ||
		FVector::DistSquared2D(Target->GetActorLocation(), GetActorLocation()) > FMath::Square(AutoAimRange);

	if (bTargetLost || Now >= NextAutoAimSearchTime)
	{
		AutoAimTarget = FindAutoAimTarget();
		NextAutoAimSearchTime = Now + AutoAimRetargetInterval;
		Target = AutoAimTarget.Get();
	}

	if (!IsValid(Target))
	{
		return false;
	}

	OutPoint = Target->GetActorLocation();
	return true;
}

// ---------------------------------------------------------------------------
// Stats panel
// ---------------------------------------------------------------------------

void ARougeCharacter::RefreshStats()
{
	if (!IsLocallyControlled() || !IsValid(RougeAttributes))
	{
		return;
	}

	int32 TeamLevel = 1;
	if (const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr)
	{
		TeamLevel = RougeGameState->GetTeamLevel();
	}

	const float MoveSpeed = GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 0.f;

	FString Stats = FString::Printf(
		TEXT("Level %d\nHP %d / %d\nArmor %d\nCrit Chance %d%%\nCrit Damage x%.2f\nMove Speed %d"),
		TeamLevel,
		FMath::RoundToInt(RougeAttributes->GetHealth()),
		FMath::RoundToInt(RougeAttributes->GetMaxHealth()),
		FMath::RoundToInt(RougeAttributes->GetArmor()),
		FMath::RoundToInt(RougeAttributes->GetCritChance()),
		RougeAttributes->GetCritDamage(),
		FMath::RoundToInt(MoveSpeed));

	if (RougeAttributes->GetShield() > 0.f)
	{
		Stats += FString::Printf(TEXT("\nShield %d"), FMath::RoundToInt(RougeAttributes->GetShield()));
	}

	OnStatsChanged(FText::FromString(Stats));
}
