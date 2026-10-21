// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EnemyBase.h"
#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Data/CharacterClassInfo.h"
#include "Net/UnrealNetwork.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Character/RougeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Actors/RougePackage.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Projectiles/ProjectileBase.h"
#include "Actors/RougeXPOrb.h"
#include "Game/RougeGameState.h"
#include "TimerManager.h"

#include <Libraries/RougeAbilitySystemLibrary.h>

AEnemyBase::AEnemyBase()
{
	bReplicates = true;

	// Spawned enemies (wave spawner) need an AI controller too, not only level-placed ones.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	XPOrbClass = ARougeXPOrb::StaticClass();

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 350.f;
		// Cheap crowd separation, no navmesh needed.
		Movement->bUseRVOAvoidance = true;
		Movement->AvoidanceConsiderationRadius = 250.f;
	}

	RougeAbilitySystemComp = CreateDefaultSubobject<URougeAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	RougeAbilitySystemComp->SetIsReplicated(true);
	RougeAbilitySystemComp->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	RougeAttributes = CreateDefaultSubobject<URougeAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AEnemyBase::GetAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

void AEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEnemyBase, bInitAttributes);
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	BindCallbacksToDependencies();
	InitAbilityActorInfo();


}

void AEnemyBase::InitAbilityActorInfo()
{
	if(IsValid(RougeAbilitySystemComp) && IsValid(RougeAttributes))
	{
		RougeAbilitySystemComp->InitAbilityActorInfo(this, this);
		if(HasAuthority())
		{
			InitClassDefaults();
			BroadcastInitialValues();
		}
	}
}

void AEnemyBase::InitClassDefaults()
{
	if(!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("'%s' CharacterTag is not valid! Please set a valid GameplayTag for the character in the class defaults."), *GetNameSafe(this));
	}
	else if(UCharacterClassInfo* ClassInfo = URougeAbilitySystemLibrary::GetCharacterClassDefaultInfo(this))
	{
		if(const FCharacterClassDefaultInfo* SelectedClass = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
		{
			if(IsValid(RougeAbilitySystemComp))
			{
				RougeAbilitySystemComp->InitializeDefaultAttributes(SelectedClass->DefaultAttributes);
				ApplyDifficultyScaling();
				RougeAbilitySystemComp->AddCharacterAbilities(SelectedClass->StartingAbilities);
				RougeAbilitySystemComp->AddCharacterPassiveAbilities(SelectedClass->PassiveAbilities);
			}

		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("'%s' Could not find default info for CharacterTag '%s' in CharacterClassInfo data asset."), *GetNameSafe(this), *CharacterTag.ToString());
		}
	}
}

void AEnemyBase::ApplyDifficultyScaling()
{
	const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr;
	if (!RougeGameState || !IsValid(RougeAbilitySystemComp) || !IsValid(RougeAttributes))
	{
		return;
	}

	const float TotalHealthMultiplier = RougeGameState->GetEnemyHealthMultiplier() * HealthMultiplier;
	if (!FMath::IsNearlyEqual(TotalHealthMultiplier, 1.f))
	{
		const float ScaledMaxHealth = RougeAttributes->GetMaxHealth() * TotalHealthMultiplier;
		RougeAbilitySystemComp->SetNumericAttributeBase(URougeAttributeSet::GetMaxHealthAttribute(), ScaledMaxHealth);
		RougeAbilitySystemComp->SetNumericAttributeBase(URougeAttributeSet::GetHealthAttribute(), ScaledMaxHealth);
	}

	AttackDamage *= RougeGameState->GetEnemyDamageMultiplier();
}

void AEnemyBase::BindCallbacksToDependencies()
{
	if(IsValid(RougeAbilitySystemComp) && IsValid(RougeAttributes))
	{
		RougeAbilitySystemComp->GetGameplayAttributeValueChangeDelegate(RougeAttributes->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.NewValue, RougeAttributes->GetMaxHealth());
			});

		if(HasAuthority())
		{
			RougeAbilitySystemComp->OnAttributesGiven.AddLambda(
			[this]()
			{
					bInitAttributes = true;
			});
		}
	}
}

void AEnemyBase::BroadcastInitialValues()
{
	if(IsValid(RougeAttributes))
	{
		OnHealthChanged(RougeAttributes->GetHealth(), RougeAttributes->GetMaxHealth());
	}
}

void AEnemyBase::OnRep_InitAttributes()
{
	BroadcastInitialValues();
}

// ---------------------------------------------------------------------------
// Death
// ---------------------------------------------------------------------------

void AEnemyBase::HandleHealthDepleted()
{
	if (!HasAuthority() || bDead)
	{
		return;
	}

	bDead = true;
	GetWorldTimerManager().ClearTimer(MeleeHitTimer);

	// Thief killed: the package falls where it died.
	if (ARougePackage* Package = CachedPackage.Get())
	{
		if (Package->GetCarrier() == this)
		{
			Package->Drop();
		}
	}
	SetActorEnableCollision(false);

	if (XPOrbClass && XPReward > 0.f)
	{
		const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
		const FVector OrbLocation = GetActorLocation() - FVector(0.f, 0.f, HalfHeight - 40.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ARougeXPOrb* Orb = GetWorld()->SpawnActor<ARougeXPOrb>(XPOrbClass, OrbLocation, FRotator::ZeroRotator, SpawnParams))
		{
			Orb->XPValue = XPReward;
		}
	}

	Destroy();
}

// ---------------------------------------------------------------------------
// AI (server only)
// ---------------------------------------------------------------------------

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !bDead)
	{
		UpdateSlow();
		TickAI(DeltaTime);
	}
}

void AEnemyBase::UpdateTarget()
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		CurrentTarget = nullptr;
		return;
	}

	const FVector MyLocation = GetActorLocation();
	ARougeCharacter* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	// Package carrier nearby: go for them instead of the nearest player.
	CachedPackage = ARougePackage::Find(GetWorld());
	if (const ARougePackage* Package = CachedPackage.Get())
	{
		ARougeCharacter* Carrier = Cast<ARougeCharacter>(Package->GetCarrier());
		if (Carrier && !Carrier->IsDead() && PackageFocusRadius > 0.f &&
			FVector::DistSquared2D(Carrier->GetActorLocation(), MyLocation) <= FMath::Square(PackageFocusRadius))
		{
			CurrentTarget = Carrier;
			return;
		}
	}

	for (const APlayerState* OtherPlayerState : GameState->PlayerArray)
	{
		ARougeCharacter* Player = OtherPlayerState ? OtherPlayerState->GetPawn<ARougeCharacter>() : nullptr;
		if (!Player || Player->IsDead())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(Player->GetActorLocation(), MyLocation);
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Player;
		}
	}

	CurrentTarget = Closest;
}

float AEnemyBase::GetEdgeDistanceTo(const AActor* Other) const
{
	const float CenterDistance = FVector::Dist2D(GetActorLocation(), Other->GetActorLocation());
	const float MyRadius = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;

	float OtherRadius = 0.f;
	if (const ACharacter* OtherCharacter = Cast<ACharacter>(Other))
	{
		OtherRadius = OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}

	return FMath::Max(0.f, CenterDistance - MyRadius - OtherRadius);
}

void AEnemyBase::ApplyStun(float Duration)
{
	if (!HasAuthority() || bDead || Duration <= 0.f)
	{
		return;
	}

	StunnedUntilTime = FMath::Max(StunnedUntilTime, GetWorld()->GetTimeSeconds() + Duration);
	GetWorldTimerManager().ClearTimer(MeleeHitTimer);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
}

bool AEnemyBase::IsStunned() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < StunnedUntilTime;
}

void AEnemyBase::ApplySlow(float SpeedMultiplier, float Duration)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!HasAuthority() || bDead || Duration <= 0.f || !Movement)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (UnslowedWalkSpeed < 0.f)
	{
		UnslowedWalkSpeed = Movement->MaxWalkSpeed;
	}

	// Strongest active slow wins; the duration is refreshed.
	SlowSpeedMultiplier = Now < SlowedUntilTime ? FMath::Min(SlowSpeedMultiplier, SpeedMultiplier) : SpeedMultiplier;
	SlowedUntilTime = FMath::Max(SlowedUntilTime, Now + Duration);
	Movement->MaxWalkSpeed = UnslowedWalkSpeed * FMath::Clamp(SlowSpeedMultiplier, 0.05f, 1.f);
}

void AEnemyBase::UpdateSlow()
{
	if (UnslowedWalkSpeed < 0.f || GetWorld()->GetTimeSeconds() < SlowedUntilTime)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = UnslowedWalkSpeed;
	}
	SlowSpeedMultiplier = 1.f;
	UnslowedWalkSpeed = -1.f;
}

void AEnemyBase::TickAI(float DeltaTime)
{
	const float Now = GetWorld()->GetTimeSeconds();

	if (Now < StunnedUntilTime)
	{
		return;
	}

	if (Now >= NextTargetSearchTime || !CurrentTarget.IsValid() || CurrentTarget->IsDead())
	{
		UpdateTarget();
		NextTargetSearchTime = Now + TargetSearchInterval;
	}

	if (TickPackageAI(DeltaTime))
	{
		return;
	}

	ARougeCharacter* Target = CurrentTarget.Get();
	if (!Target)
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	const FVector Direction = ToTarget.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	// Face the target (bUseControllerRotationYaw follows the control rotation).
	if (AController* MyController = GetController())
	{
		const FRotator Desired = Direction.Rotation();
		const FRotator NewRotation = TurnInterpSpeed > 0.f
			? FMath::RInterpTo(MyController->GetControlRotation(), Desired, DeltaTime, TurnInterpSpeed)
			: Desired;
		MyController->SetControlRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
	}

	const float EdgeDistance = GetEdgeDistanceTo(Target);

	// Melee keeps pushing in; ranged holds its preferred distance.
	const float StopDistance = (AttackType == EEnemyAttackType::Ranged) ? PreferredDistance : AttackRange * 0.5f;
	if (EdgeDistance > StopDistance)
	{
		AddMovementInput(Direction, 1.f);
	}

	if (EdgeDistance <= AttackRange && Now >= NextAttackTime)
	{
		NextAttackTime = Now + AttackCooldown;
		StartAttack();
	}
}

void AEnemyBase::MoveAndFace(const FVector& Direction, float DeltaTime)
{
	if (Direction.IsNearlyZero())
	{
		return;
	}

	if (AController* MyController = GetController())
	{
		const FRotator Desired = Direction.Rotation();
		const FRotator NewRotation = TurnInterpSpeed > 0.f
			? FMath::RInterpTo(MyController->GetControlRotation(), Desired, DeltaTime, TurnInterpSpeed)
			: Desired;
		MyController->SetControlRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
	}
	AddMovementInput(Direction, 1.f);
}

bool AEnemyBase::TickPackageAI(float DeltaTime)
{
	const ARougePackage* Package = CachedPackage.Get();
	if (!Package || PackageFocusRadius <= 0.f)
	{
		return false;
	}

	const FVector MyLocation = GetActorLocation();

	// Carrying it: run away from the closest player, no attacking.
	if (Package->GetCarrier() == this)
	{
		FVector Away = FVector::ZeroVector;
		float ClosestDistSq = TNumericLimits<float>::Max();
		for (TActorIterator<ARougeCharacter> It(GetWorld()); It; ++It)
		{
			const ARougeCharacter* Player = *It;
			if (!Player->IsDead())
			{
				const float DistSq = FVector::DistSquared2D(Player->GetActorLocation(), MyLocation);
				if (DistSq < ClosestDistSq)
				{
					ClosestDistSq = DistSq;
					Away = MyLocation - Player->GetActorLocation();
				}
			}
		}
		Away.Z = 0.f;
		MoveAndFace(Away.GetSafeNormal(), DeltaTime);
		return true;
	}

	// On the ground nearby: go grab it (the package picks up whoever touches it).
	if (Package->IsOnGround() && Package->CanEnemiesCarry())
	{
		FVector ToPackage = Package->GetActorLocation() - MyLocation;
		ToPackage.Z = 0.f;
		if (ToPackage.SizeSquared() <= FMath::Square(PackageFocusRadius))
		{
			MoveAndFace(ToPackage.GetSafeNormal(), DeltaTime);
			return true;
		}
	}

	return false;
}

void AEnemyBase::StartAttack()
{
	MulticastPlayAttackMontage();

	if (AttackType == EEnemyAttackType::Ranged)
	{
		FireProjectile();
	}
	else if (MeleeHitDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(MeleeHitTimer, this, &ThisClass::ApplyMeleeHit, MeleeHitDelay, false);
	}
	else
	{
		ApplyMeleeHit();
	}
}

void AEnemyBase::ApplyMeleeHit()
{
	ARougeCharacter* Target = CurrentTarget.Get();
	if (bDead || !Target || Target->IsDead())
	{
		return;
	}

	// Small tolerance so a target that just stepped back still gets hit.
	if (GetEdgeDistanceTo(Target) <= AttackRange * 1.25f)
	{
		ApplyDamageTo(Target);
	}
}

void AEnemyBase::ApplyDamageTo(AActor* Target)
{
	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Target);
	UAbilitySystemComponent* TargetASC = TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC || !DamageEffect)
	{
		return;
	}

	FDamageEffectInfo Info;
	Info.AvatarActor = this;
	Info.DamageEffect = DamageEffect;
	Info.BaseDamage = AttackDamage;
	Info.AbilityLevel = 1.f;
	Info.SourceASC = RougeAbilitySystemComp;
	Info.TargetASC = TargetASC;

	URougeAbilitySystemLibrary::ApplyDamageEffect(Info);
}

void AEnemyBase::FireProjectile()
{
	ARougeCharacter* Target = CurrentTarget.Get();
	if (!Target || !RangedProjectile.ProjectileClass)
	{
		return;
	}

	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	Direction.Normalize();

	const float MyRadius = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
	const FVector SpawnLocation = GetActorLocation() + Direction * (MyRadius + 30.f) + FVector(0.f, 0.f, 40.f);

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SpawnLocation);
	SpawnTransform.SetRotation(Direction.Rotation().Quaternion());

	if (AProjectileBase* Projectile = GetWorld()->SpawnActorDeferred<AProjectileBase>(
		RangedProjectile.ProjectileClass, SpawnTransform, this, this))
	{
		Projectile->SetProjectileParams(RangedProjectile);

		FDamageEffectInfo Info;
		Info.AvatarActor = this;
		Info.DamageEffect = DamageEffect;
		Info.BaseDamage = AttackDamage;
		Info.AbilityLevel = 1.f;
		Info.SourceASC = RougeAbilitySystemComp;
		Projectile->DamageEffectInfo = Info;

		Projectile->FinishSpawning(SpawnTransform);
	}
}

void AEnemyBase::MulticastPlayAttackMontage_Implementation()
{
	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}
}
