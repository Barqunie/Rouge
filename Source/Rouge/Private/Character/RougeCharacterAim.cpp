// Aim mode (mouse / auto aim) for ARougeCharacter. Runs on the owning client only.

#include "Character/RougeCharacter.h"

#include "Character/EnemyBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
