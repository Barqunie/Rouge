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

TArray<FRougeStatLine> ARougeCharacter::GetStatLines() const
{
	TArray<FRougeStatLine> Lines;
	if (!IsValid(RougeAttributes))
	{
		return Lines;
	}

	auto AddLine = [&Lines](const TCHAR* Label, const FString& Value)
	{
		FRougeStatLine& Line = Lines.AddDefaulted_GetRef();
		Line.Label = FText::FromString(Label);
		Line.Value = FText::FromString(Value);
	};

	const float MoveSpeed = GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 0.f;

	// Damage and AttackSpeed are multipliers (1 = 100%); 0 means the attribute was never set.
	auto AsPercent = [](float Multiplier)
	{
		return FString::Printf(TEXT("%d%%"), FMath::RoundToInt((Multiplier > 0.f ? Multiplier : 1.f) * 100.f));
	};

	AddLine(TEXT("Damage"), AsPercent(RougeAttributes->GetDamage()));
	AddLine(TEXT("Attack Speed"), AsPercent(RougeAttributes->GetAttackSpeed()));
	AddLine(TEXT("Armor"), FString::FromInt(FMath::RoundToInt(RougeAttributes->GetArmor())));
	AddLine(TEXT("Damage Taken"), FString::Printf(TEXT("-%d%%"), FMath::RoundToInt(RougeAttributes->GetDamageReduction())));
	AddLine(TEXT("Crit Chance"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt(RougeAttributes->GetCritChance())));
	AddLine(TEXT("Crit Damage"), FString::Printf(TEXT("x%.2f"), RougeAttributes->GetCritDamage()));
	AddLine(TEXT("Life Steal"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt(RougeAttributes->GetLifeSteal())));
	AddLine(TEXT("Move Speed"), FString::FromInt(FMath::RoundToInt(MoveSpeed)));

	if (RougeAttributes->GetShield() > 0.f)
	{
		AddLine(TEXT("Shield"), FString::FromInt(FMath::RoundToInt(RougeAttributes->GetShield())));
	}

	return Lines;
}

void ARougeCharacter::RefreshStats()
{
	if (!IsLocallyControlled() || !IsValid(RougeAttributes))
	{
		return;
	}

	FString Stats;
	for (const FRougeStatLine& Line : GetStatLines())
	{
		if (!Stats.IsEmpty())
		{
			Stats += TEXT("\n");
		}
		Stats += Line.Label.ToString() + TEXT(" ") + Line.Value.ToString();
	}

	OnStatsChanged(FText::FromString(Stats));
}
