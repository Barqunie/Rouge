#include "AbilitySystem/Abilities/MeleeDamageAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/RougeGameplayTag.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include "TimerManager.h"

UMeleeDamageAbility::UMeleeDamageAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UMeleeDamageAbility::InitMeleeData()
{
	if (IsValid(SourceActor) && IsValid(SourceASC))
	{
		return true;
	}

	SourceActor = GetAvatarActorFromActorInfo();
	if (!IsValid(SourceActor))
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility: SourceActor is null"));
		return false;
	}

	SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);
	}

	if (!IsValid(SourceASC))
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility: SourceASC is null"));
		return false;
	}

	// Optional: the tagged component only provides the hit shape. Its own collision is not used.
	HitComponent = nullptr;
	TArray<UActorComponent*> FoundComponents =
		SourceActor->GetComponentsByTag(UPrimitiveComponent::StaticClass(), HitComponentTag);

	if (FoundComponents.Num() > 0)
	{
		HitComponent = Cast<UPrimitiveComponent>(FoundComponents[0]);
		if (IsValid(HitComponent))
		{
			HitComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	return true;
}

void UMeleeDamageAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	HitActors.Empty();

	// Set up before Super: Super runs the Blueprint ActivateAbility graph, which may open a window right away.
	if (bEnableMeleeDamage)
	{
		InitMeleeData();

		if (bListenForAnimNotifyEvents)
		{
			if (UAbilityTask_WaitGameplayEvent* WaitOpen = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, RougeGameplayTags::Event::Melee_WindowOpen, nullptr, false, true))
			{
				WaitOpen->EventReceived.AddDynamic(this, &ThisClass::OnWindowOpenEvent);
				WaitOpen->ReadyForActivation();
			}

			if (UAbilityTask_WaitGameplayEvent* WaitClose = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, RougeGameplayTags::Event::Melee_WindowClose, nullptr, false, true))
			{
				WaitClose->EventReceived.AddDynamic(this, &ThisClass::OnWindowCloseEvent);
				WaitClose->ReadyForActivation();
			}

			if (UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, RougeGameplayTags::Event::Melee_Hit, nullptr, false, true))
			{
				WaitHit->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
				WaitHit->ReadyForActivation();
			}
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMeleeDamageAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	DamageWindowClose();

	SourceActor = nullptr;
	HitComponent = nullptr;
	SourceASC = nullptr;
	HitActors.Empty();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMeleeDamageAbility::DamageWindowOpen()
{
	if (!bEnableMeleeDamage || !InitMeleeData())
	{
		return;
	}

	// Damage is server-authoritative; clients only play the animation.
	if (!SourceActor->HasAuthority())
	{
		return;
	}

	HitActors.Empty();

	// Hit whatever is already inside the shape, then keep checking while the window is open.
	CheckMeleeHits();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HitCheckTimer, this, &ThisClass::CheckMeleeHits, HitCheckInterval, true);
	}
}

void UMeleeDamageAbility::DamageWindowClose()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitCheckTimer);
	}

	HitActors.Empty();
}

void UMeleeDamageAbility::PerformMeleeHit()
{
	if (!bEnableMeleeDamage || !InitMeleeData() || !SourceActor->HasAuthority())
	{
		return;
	}

	// A single hit is its own window: don't let a previous hit block this one,
	// but keep the hit list if a timed window is currently open.
	const bool bWindowOpen = GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(HitCheckTimer);
	if (!bWindowOpen)
	{
		HitActors.Empty();
	}

	CheckMeleeHits();
}

void UMeleeDamageAbility::CheckMeleeHits()
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(SourceActor) || !SourceActor->HasAuthority())
	{
		return;
	}

	FVector ShapeLocation;
	FQuat ShapeRotation;
	FCollisionShape Shape;
	if (!GetHitShape(ShapeLocation, ShapeRotation, Shape))
	{
		return;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeHit), false, SourceActor);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, ShapeLocation, ShapeRotation, ObjectParams, Shape, QueryParams);

	bool bHitSomething = false;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		if (!IsValid(OtherActor) || OtherActor == SourceActor || HitActors.Contains(OtherActor))
		{
			continue;
		}

		if (!OtherActor->ActorHasTag(EnemyTag))
		{
			continue;
		}

		HitActors.Add(OtherActor);
		ApplyMeleeDamage(OtherActor);
		bHitSomething = true;
	}

	if (bDrawDebugHitShape)
	{
		DrawDebugHitShape(ShapeLocation, ShapeRotation, Shape, bHitSomething);
	}
}

bool UMeleeDamageAbility::GetHitShape(FVector& OutLocation, FQuat& OutRotation, FCollisionShape& OutShape) const
{
	if (IsValid(HitComponent))
	{
		OutShape = HitComponent->GetCollisionShape();
		if (!OutShape.IsNearlyZero())
		{
			OutLocation = HitComponent->GetComponentLocation();
			OutRotation = HitComponent->GetComponentQuat();
			return true;
		}
	}

	if (!IsValid(SourceActor))
	{
		return false;
	}

	// Fallback: horizontal capsule lying along the actor's forward direction.
	FVector Forward = SourceActor->GetActorForwardVector();
	Forward.Z = 0.f;
	if (!Forward.Normalize())
	{
		return false;
	}

	const float HalfHeight = FMath::Max(FallbackHitLength * 0.5f, FallbackHitRadius);

	OutLocation = SourceActor->GetActorLocation() + Forward * (FallbackHitForwardOffset + FallbackHitLength * 0.5f);
	OutRotation = FRotationMatrix::MakeFromZ(Forward).ToQuat();
	OutShape = FCollisionShape::MakeCapsule(FallbackHitRadius, HalfHeight);
	return true;
}

void UMeleeDamageAbility::ApplyMeleeDamage(AActor* TargetActor)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!IsValid(TargetASC))
	{
		return;
	}

	FDamageEffectInfo Info;
	if (DamageEffectInfo.DamageEffect)
	{
		// Explicit override set from Blueprint.
		Info = DamageEffectInfo;
	}
	else
	{
		// Ability defaults (DamageEffect + BaseDamage curve).
		CaptureDamageEffectInfo(nullptr, Info);
	}

	if (!Info.DamageEffect)
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility (%s): no DamageEffect set on the ability."), *GetNameSafe(this));
		return;
	}

	Info.SourceASC = SourceASC;
	Info.AvatarActor = SourceActor;
	Info.TargetASC = TargetASC;

	if (Info.AbilityLevel <= 0.f)
	{
		Info.AbilityLevel = 1.f;
	}

	URougeAbilitySystemLibrary::ApplyDamageEffect(Info);
}

void UMeleeDamageAbility::DrawDebugHitShape(const FVector& Location, const FQuat& Rotation, const FCollisionShape& Shape, bool bHitSomething) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FColor Color = bHitSomething ? FColor::Red : FColor::Green;
	const float Duration = HitCheckInterval + 0.05f;

	if (Shape.IsCapsule())
	{
		DrawDebugCapsule(World, Location, Shape.GetCapsuleHalfHeight(), Shape.GetCapsuleRadius(), Rotation, Color, false, Duration);
	}
	else if (Shape.IsBox())
	{
		DrawDebugBox(World, Location, Shape.GetBox(), Rotation, Color, false, Duration);
	}
	else if (Shape.IsSphere())
	{
		DrawDebugSphere(World, Location, Shape.GetSphereRadius(), 12, Color, false, Duration);
	}
}

void UMeleeDamageAbility::OnWindowOpenEvent(FGameplayEventData Payload)
{
	DamageWindowOpen();
}

void UMeleeDamageAbility::OnWindowCloseEvent(FGameplayEventData Payload)
{
	DamageWindowClose();
}

void UMeleeDamageAbility::OnHitEvent(FGameplayEventData Payload)
{
	PerformMeleeHit();
}
