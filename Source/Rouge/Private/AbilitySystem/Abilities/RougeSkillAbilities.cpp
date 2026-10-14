// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/RougeSkillAbilities.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Actors/RougeAreaIndicator.h"
#include "Animation/AnimMontage.h"
#include "Character/EnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Interfaces/RougeAbilitySystemInterface.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include "Projectiles/ProjectileBase.h"
#include "TimerManager.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// URougeSkillAbility
// ---------------------------------------------------------------------------

URougeSkillAbility::URougeSkillAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultBurst(TEXT("/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst"));
	if (DefaultBurst.Succeeded())
	{
		ImpactVFX = DefaultBurst.Object;
	}
}

bool URougeSkillAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (GetCooldownRemaining() > 0.f)
	{
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

float URougeSkillAbility::GetCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	return FMath::Max(0.f, LastActivationTime + Cooldown - World->GetTimeSeconds());
}

void URougeSkillAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	LastActivationTime = GetWorld()->GetTimeSeconds();

	if (SkillMontage)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			// Replicates to simulated proxies through the ASC.
			ASC->PlayMontage(this, ActivationInfo, SkillMontage, 1.f);
		}
	}

	ActivateSkill();
}

void URougeSkillAbility::FinishSkill(bool bWasCancelled)
{
	if (IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, bWasCancelled);
	}
}

bool URougeSkillAbility::IsServer() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar && Avatar->HasAuthority();
}

FVector URougeSkillAbility::GetAvatarForward2D() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return FVector::ForwardVector;
	}

	FVector Forward = Avatar->GetActorForwardVector();
	Forward.Z = 0.f;
	return Forward.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
}

void URougeSkillAbility::DamageEnemy(AActor* Enemy, float StunDuration)
{
	AEnemyBase* EnemyCharacter = Cast<AEnemyBase>(Enemy);
	if (!IsServer() || !EnemyCharacter || EnemyCharacter->IsDead())
	{
		return;
	}

	FDamageEffectInfo Info;
	CaptureDamageEffectInfo(EnemyCharacter, Info);
	if (Info.DamageEffect && Info.BaseDamage > 0.f)
	{
		URougeAbilitySystemLibrary::ApplyDamageEffect(Info);
	}

	// The hit may have killed (destroyed) it.
	if (StunDuration > 0.f && IsValid(EnemyCharacter) && !EnemyCharacter->IsDead())
	{
		EnemyCharacter->ApplyStun(StunDuration);
		// Stunned enemies don't move, so a static marker under them lasts the whole stun.
		ARougeAreaIndicator::SpawnIndicator(GetWorld(), EnemyCharacter->GetActorLocation(), 60.f, StunMarkerColor, StunDuration);
	}
}

int32 URougeSkillAbility::DamageEnemiesInRadius(const FVector& Center, float Radius, float StunDuration)
{
	UWorld* World = GetWorld();
	if (!IsServer() || !World)
	{
		return 0;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillArea), false, GetAvatarActorFromActorInfo());

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeCapsule(Radius, 200.f), QueryParams);

	TSet<AActor*> Hit;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (Actor && Actor->IsA<AEnemyBase>() && !Hit.Contains(Actor))
		{
			Hit.Add(Actor);
			DamageEnemy(Actor, StunDuration);
		}
	}

	return Hit.Num();
}

void URougeSkillAbility::SpawnIndicator(const FVector& Center, float Radius, float Duration) const
{
	if (IsServer())
	{
		FLinearColor Telegraph = IndicatorColor;
		Telegraph.A *= 0.5f;
		ARougeAreaIndicator::SpawnIndicator(GetWorld(), Center, Radius, Telegraph, Duration);
	}
}

void URougeSkillAbility::SpawnImpactEffect(const FVector& Center, float Radius) const
{
	if (IsServer())
	{
		ARougeAreaIndicator::SpawnIndicator(GetWorld(), Center, Radius, IndicatorColor, 0.5f, true, ImpactVFX, ImpactVFXScale);
	}
}

// ---------------------------------------------------------------------------
// URougeAreaSkillAbility
// ---------------------------------------------------------------------------

void URougeAreaSkillAbility::ActivateSkill()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		FinishSkill(true);
		return;
	}

	ImpactCenter = Avatar->GetActorLocation();
	if (Center == ERougeAreaCenter::InFront)
	{
		ImpactCenter += GetAvatarForward2D() * ForwardDistance;
	}

	// Telegraph circle until the impact.
	if (ImpactDelay > 0.f)
	{
		SpawnIndicator(ImpactCenter, Radius, ImpactDelay);
	}

	if (ImpactDelay > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(ImpactTimer, this, &ThisClass::Impact, ImpactDelay, false);
	}
	else
	{
		Impact();
	}
}

void URougeAreaSkillAbility::Impact()
{
	SpawnImpactEffect(ImpactCenter, Radius);
	DamageEnemiesInRadius(ImpactCenter, Radius, StunDuration);
	FinishSkill();
}

// ---------------------------------------------------------------------------
// URougeDashSkillAbility
// ---------------------------------------------------------------------------

void URougeDashSkillAbility::ActivateSkill()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		FinishSkill(true);
		return;
	}

	const FVector Direction = GetAvatarForward2D();
	const FVector Start = Character->GetActorLocation();
	const FVector End = Start + Direction * DashDistance;

	// Server: damage everything along the dash path once.
	if (IsServer())
	{
		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DashSkill), false, Character);

		TArray<FHitResult> Hits;
		GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectParams,
			FCollisionShape::MakeSphere(HitRadius), QueryParams);

		TSet<AActor*> Damaged;
		for (const FHitResult& Hit : Hits)
		{
			AActor* Actor = Hit.GetActor();
			if (Actor && Actor->IsA<AEnemyBase>() && !Damaged.Contains(Actor))
			{
				Damaged.Add(Actor);
				DamageEnemy(Actor);
			}
		}

		SpawnImpactEffect(Start, HitRadius);
		SpawnImpactEffect(End, HitRadius * 1.5f);
	}

	// Dash through enemies.
	SetIgnorePawns(true);

	// Predicted root motion: runs on the owning client and the server.
	UAbilityTask_ApplyRootMotionConstantForce* DashTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, NAME_None, Direction, DashDistance / DashDuration, DashDuration,
		false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, false);

	if (DashTask)
	{
		DashTask->OnFinish.AddDynamic(this, &ThisClass::OnDashFinished);
		DashTask->ReadyForActivation();
	}
	else
	{
		FinishSkill();
	}
}

void URougeDashSkillAbility::OnDashFinished()
{
	FinishSkill();
}

void URougeDashSkillAbility::SetIgnorePawns(bool bIgnore)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || bIgnoringPawns == bIgnore)
	{
		return;
	}

	bIgnoringPawns = bIgnore;
	if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, bIgnore ? ECR_Ignore : ECR_Block);
	}
}

void URougeDashSkillAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	SetIgnorePawns(false);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
// URougeShieldSkillAbility
// ---------------------------------------------------------------------------

void URougeShieldSkillAbility::ActivateSkill()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsServer() && ASC)
	{
		ASC->SetNumericAttributeBase(URougeAttributeSet::GetShieldAttribute(), ShieldAmount);

		// Lambda timer (not bound to this ability) so it survives the ability ending right away.
		FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		TimerManager.ClearTimer(ShieldExpireTimer);
		TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
		TimerManager.SetTimer(ShieldExpireTimer, FTimerDelegate::CreateWeakLambda(ASC, [WeakASC]()
		{
			if (WeakASC.IsValid())
			{
				WeakASC->SetNumericAttributeBase(URougeAttributeSet::GetShieldAttribute(), 0.f);
			}
		}), ShieldDuration, false);

		if (const AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			SpawnImpactEffect(Avatar->GetActorLocation(), 150.f);
		}
	}

	FinishSkill();
}

// ---------------------------------------------------------------------------
// URougeVolleySkillAbility
// ---------------------------------------------------------------------------

void URougeVolleySkillAbility::ActivateSkill()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsServer() || !Avatar || !Projectile.ProjectileClass)
	{
		FinishSkill();
		return;
	}

	FVector SpawnLocation = Avatar->GetActorLocation() + GetAvatarForward2D() * 60.f;
	if (Avatar->Implements<URougeAbilitySystemInterface>())
	{
		if (const USceneComponent* SpawnPoint = IRougeAbilitySystemInterface::Execute_GetDynamicSpawnPoint(Avatar))
		{
			SpawnLocation = SpawnPoint->GetComponentLocation();
		}
	}

	const float BaseYaw = GetAvatarForward2D().Rotation().Yaw;
	const int32 Count = FMath::Max(1, NumProjectiles);
	const float Step = Count > 1 ? SpreadAngle / (Count - 1) : 0.f;
	const float StartYaw = BaseYaw - (Count > 1 ? SpreadAngle * 0.5f : 0.f);

	FDamageEffectInfo DamageInfo;
	CaptureDamageEffectInfo(nullptr, DamageInfo);

	SpawnImpactEffect(SpawnLocation, 80.f);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FRotator Rotation(0.f, StartYaw + Step * Index, 0.f);
		const FTransform SpawnTransform(Rotation, SpawnLocation);

		if (AProjectileBase* SpawnedProjectile = GetWorld()->SpawnActorDeferred<AProjectileBase>(
			Projectile.ProjectileClass, SpawnTransform, Avatar, Cast<APawn>(Avatar)))
		{
			SpawnedProjectile->SetProjectileParams(Projectile);
			SpawnedProjectile->DamageEffectInfo = DamageInfo;
			SpawnedProjectile->FinishSpawning(SpawnTransform);
		}
	}

	FinishSkill();
}
