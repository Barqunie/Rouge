#include "AbilitySystem/Abilities/RougePassiveAbilities.h"

#include "AbilitySystem/RougeAbilityTypes.h"
#include "Actors/RougeAreaIndicator.h"
#include "Character/CharacterBase.h"
#include "Character/EnemyBase.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include "Projectiles/RougeBoomerang.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// URougePassiveAbility
// ---------------------------------------------------------------------------

URougePassiveAbility::URougePassiveAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// Same damage effect as the attacks / skills (SetByCaller damage through ExecCalc_Damage).
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectClass(TEXT("/Game/ThirdPerson/AbilitySystem/Globals/GE_Damage"));
	if (DamageEffectClass.Succeeded())
	{
		DamageEffect = DamageEffectClass.Class;
	}
}

void URougePassiveAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Stays active; the timer is cleared when the ability ends (death cancels all abilities).
	SchedulePulse();
}

void URougePassiveAbility::SchedulePulse()
{
	if (UWorld* World = GetWorld())
	{
		// Re-armed every pulse so a level change (new stack) takes effect right away.
		World->GetTimerManager().SetTimer(PulseTimer, this, &ThisClass::HandlePulseTimer, GetInterval(), false);
	}
}

void URougePassiveAbility::HandlePulseTimer()
{
	if (!IsActive())
	{
		return;
	}

	if (IsOwnerAlive())
	{
		Pulse();
	}
	SchedulePulse();
}

float URougePassiveAbility::GetInterval() const
{
	const int32 ExtraLevels = FMath::Max(0, GetAbilityLevel() - 1);
	return FMath::Max(0.2f, BaseInterval * FMath::Pow(IntervalScalePerLevel, ExtraLevels));
}

float URougePassiveAbility::GetDamageScale() const
{
	return 1.f + DamageBonusPerLevel * FMath::Max(0, GetAbilityLevel() - 1);
}

bool URougePassiveAbility::IsOwnerAlive() const
{
	const ACharacterBase* Avatar = Cast<ACharacterBase>(GetAvatarActorFromActorInfo());
	return Avatar && !Avatar->IsDead();
}

void URougePassiveAbility::DamageEnemy(AActor* Enemy)
{
	AEnemyBase* EnemyCharacter = Cast<AEnemyBase>(Enemy);
	if (!EnemyCharacter || EnemyCharacter->IsDead())
	{
		return;
	}

	FDamageEffectInfo Info;
	CaptureDamageEffectInfo(EnemyCharacter, Info);
	Info.BaseDamage *= GetDamageScale();
	if (Info.DamageEffect && Info.BaseDamage > 0.f)
	{
		URougeAbilitySystemLibrary::ApplyDamageEffect(Info);
	}
}

TArray<AEnemyBase*> URougePassiveAbility::FindEnemiesInRadius(const FVector& Center, float Radius) const
{
	TArray<AEnemyBase*> Enemies;
	const float RadiusSq = FMath::Square(Radius);

	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		AEnemyBase* Enemy = *It;
		if (IsValid(Enemy) && !Enemy->IsDead() && FVector::DistSquared2D(Enemy->GetActorLocation(), Center) <= RadiusSq)
		{
			Enemies.Add(Enemy);
		}
	}

	Enemies.Sort([&Center](const AEnemyBase& A, const AEnemyBase& B)
	{
		return FVector::DistSquared2D(A.GetActorLocation(), Center) < FVector::DistSquared2D(B.GetActorLocation(), Center);
	});
	return Enemies;
}

// ---------------------------------------------------------------------------
// URougeBoomerangPassiveAbility
// ---------------------------------------------------------------------------

URougeBoomerangPassiveAbility::URougeBoomerangPassiveAbility()
{
	BaseInterval = 3.f;
	BaseDamage = FScalableFloat(14.f);
	BoomerangClass = ARougeBoomerang::StaticClass();
}

void URougeBoomerangPassiveAbility::Pulse()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !BoomerangClass)
	{
		return;
	}

	const FVector Start = Avatar->GetActorLocation();
	const TArray<AEnemyBase*> Enemies = FindEnemiesInRadius(Start, Range);
	if (Enemies.Num() == 0)
	{
		return;
	}

	FVector Direction = Enemies[0]->GetActorLocation() - Start;
	Direction.Z = 0.f;
	const float DistanceToTarget = Direction.Size();
	Direction = Direction.GetSafeNormal(UE_SMALL_NUMBER, Avatar->GetActorForwardVector());

	// Fly a bit past the target so it gets hit on the way out and on the way back.
	const float TravelDistance = FMath::Clamp(DistanceToTarget + 150.f, 300.f, Range);

	FDamageEffectInfo DamageInfo;
	CaptureDamageEffectInfo(nullptr, DamageInfo);
	DamageInfo.BaseDamage *= GetDamageScale();

	const FTransform SpawnTransform(Direction.Rotation(), Start);
	if (ARougeBoomerang* Boomerang = GetWorld()->SpawnActorDeferred<ARougeBoomerang>(
		BoomerangClass, SpawnTransform, Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Boomerang->Init(DamageInfo, Direction, TravelDistance, Speed);
		Boomerang->FinishSpawning(SpawnTransform);
	}
}

// ---------------------------------------------------------------------------
// URougeFrostPulsePassiveAbility
// ---------------------------------------------------------------------------

URougeFrostPulsePassiveAbility::URougeFrostPulsePassiveAbility()
{
	BaseInterval = 4.f;
	BaseDamage = FScalableFloat(5.f);
	DamageBonusPerLevel = 0.5f;
}

void URougeFrostPulsePassiveAbility::Pulse()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	const FVector Center = Avatar->GetActorLocation();
	const float PulseRadius = Radius * (1.f + RadiusBonusPerLevel * FMath::Max(0, GetAbilityLevel() - 1));

	ARougeAreaIndicator::SpawnIndicator(GetWorld(), Center, PulseRadius, PulseColor, 0.5f, true);

	for (AEnemyBase* Enemy : FindEnemiesInRadius(Center, PulseRadius))
	{
		DamageEnemy(Enemy);
		// The hit may have killed (destroyed) it.
		if (IsValid(Enemy) && !Enemy->IsDead())
		{
			Enemy->ApplySlow(SlowSpeedMultiplier, SlowDuration);
		}
	}
}
