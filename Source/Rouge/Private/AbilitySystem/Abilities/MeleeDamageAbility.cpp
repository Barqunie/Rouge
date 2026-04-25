#include "AbilitySystem/Abilities/MeleeDamageAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Libraries/RougeAbilitySystemLibrary.h"

UMeleeDamageAbility::UMeleeDamageAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UMeleeDamageAbility::InitMeleeData()
{
	if (IsValid(SourceActor) && IsValid(HitComponent) && IsValid(SourceASC))
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

	TArray<UActorComponent*> FoundComponents =
		SourceActor->GetComponentsByTag(UPrimitiveComponent::StaticClass(), HitComponentTag);

	if (FoundComponents.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility: No component with tag '%s' found on %s"),
			*HitComponentTag.ToString(), *SourceActor->GetName());
		return false;
	}

	HitComponent = Cast<UPrimitiveComponent>(FoundComponents[0]);
	if (!IsValid(HitComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility: Tagged component is not a PrimitiveComponent"));
		return false;
	}

	// Library'nin beklediði alanlarý burada hazýrla
	DamageEffectInfo.SourceASC = SourceASC;
	DamageEffectInfo.AvatarActor = SourceActor;

	// 0 kalýrsa spec level saçmalar; güvenli olsun
	if (DamageEffectInfo.AbilityLevel <= 0.f)
	{
		DamageEffectInfo.AbilityLevel = 1.f;
	}

	return true;
}

void UMeleeDamageAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!bEnableMeleeDamage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	InitMeleeData();
	HitActors.Empty();
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
	if (!InitMeleeData())
	{
		return;
	}

	if (!SourceActor->HasAuthority())
	{
		return;
	}

	HitActors.Empty();

	HitComponent->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&ThisClass::OnMeleeOverlapBegin
	);

	HitComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Pencere açýldýðýnda zaten iç içeyseler onu da vur
	TArray<AActor*> OverlappingActors;
	HitComponent->GetOverlappingActors(OverlappingActors);

	for (AActor* OtherActor : OverlappingActors)
	{
		OnMeleeOverlapBegin(HitComponent, OtherActor, nullptr, 0, false, FHitResult());
	}
}

void UMeleeDamageAbility::DamageWindowClose()
{
	if (!IsValid(HitComponent))
	{
		return;
	}

	if (IsValid(SourceActor) && !SourceActor->HasAuthority())
	{
		return;
	}

	HitComponent->OnComponentBeginOverlap.RemoveDynamic(
		this,
		&ThisClass::OnMeleeOverlapBegin
	);

	HitComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitActors.Empty();
}

void UMeleeDamageAbility::OnMeleeOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!IsValid(OtherActor) || OtherActor == SourceActor)
	{
		return;
	}

	if (HitActors.Contains(OtherActor))
	{
		return;
	}

	if (!OtherActor->ActorHasTag(EnemyTag))
	{
		return;
	}

	if (!IsValid(SourceASC))
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility: SourceASC is null"));
		return;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);

	if (!IsValid(TargetASC))
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility: TargetASC is null for %s"), *OtherActor->GetName());
		return;
	}

	if (!DamageEffectInfo.DamageEffect)
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeDamageAbility: DamageEffect is null"));
		return;
	}

	// Library'nin kullandýðý tüm alanlarý doldur
	DamageEffectInfo.SourceASC = SourceASC;
	DamageEffectInfo.TargetASC = TargetASC;
	DamageEffectInfo.AvatarActor = SourceActor;

	if (DamageEffectInfo.AbilityLevel <= 0.f)
	{
		DamageEffectInfo.AbilityLevel = 1.f;
	}

	HitActors.Add(OtherActor);

	URougeAbilitySystemLibrary::ApplyDamageEffect(DamageEffectInfo);
}