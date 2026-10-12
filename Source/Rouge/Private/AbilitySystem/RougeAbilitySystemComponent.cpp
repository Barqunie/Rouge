// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/RougeGameplayAbility.h"
#include <AbilitySystem/Abilities/ProjectileAbility.h>

namespace
{
	// Spec.ActivationInfo is deprecated for instanced abilities; read it from the active instance instead.
	FPredictionKey GetActivePredictionKey(const FGameplayAbilitySpec& Spec)
	{
		const TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
		return Instances.Num() > 0 ? Instances.Last()->GetCurrentActivationInfo().GetActivationPredictionKey() : FPredictionKey();
	}
}




void URougeAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilitiesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : AbilitiesToGrant)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);

		if(URougeGameplayAbility* RougeAbility = Cast<URougeGameplayAbility>(AbilitySpec.Ability))
		{
			if (RougeAbility->InputTag.IsValid())
			{
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(RougeAbility->InputTag);
				GiveAbility(AbilitySpec);
			}
			else 
			{
				GiveAbilityAndActivateOnce(AbilitySpec);
				//eger local predicted ise input yok iken calýsmaz
				//skilli inputsuz yapýcaksan abilitynin bpsinden local predicted deðiþtir.
			}
		}
	}
}



void URougeAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& PassivesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : PassivesToGrant)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);
		GiveAbilityAndActivateOnce(AbilitySpec);

	}
}

void URougeAbilitySystemComponent::InitializeDefaultAttributes(const TSubclassOf<UGameplayEffect>& AttributeEffect)
{
	checkf(
		AttributeEffect,
		TEXT("No Valid Default Attributes For This character: %s"),
		*GetAvatarActor()->GetName()
	);




	const FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(AttributeEffect, 1.f, ContextHandle);
	ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	OnAttributesGiven.Broadcast();


}

void URougeAbilitySystemComponent::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if(!InputTag.IsValid()){return;}

	ABILITYLIST_SCOPE_LOCK();

	for(const FGameplayAbilitySpec& Spec :GetActivatableAbilities())
	{
			if(Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				if(!Spec.IsActive())
				{
					TryActivateAbility(Spec.Handle);
				}
				else
				{
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle,GetActivePredictionKey(Spec));
				}
			}

	}
}

void URougeAbilitySystemComponent::AbilityInputReleased(const FGameplayTag& InputTag)
{

	if(!InputTag.IsValid()){return;}
	ABILITYLIST_SCOPE_LOCK();
	for(const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
			if(Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle,GetActivePredictionKey(Spec));
			}
	}
}

void URougeAbilitySystemComponent::SetDynamicProjectile(const FGameplayTag& ProjectileTag, int32 AbilityLevel)
{
	if (!ProjectileTag.IsValid()) return;

	if (!GetAvatarActor()->HasAuthority())
	{
		ServerSetDynamicProjectile(ProjectileTag,AbilityLevel);
		return;
	}


	if (ActiveProjectileAbility.IsValid())
	{
		ClearAbility(ActiveProjectileAbility);
	}

	if (IsValid(DynamicProjectileAbility))
	{
		FGameplayAbilitySpec Spec = FGameplayAbilitySpec(DynamicProjectileAbility, AbilityLevel);
		// Spec.Ability is the CDO (shared by all players) - only read from it here.
		if (const UProjectileAbility* ProjectileAbilityCDO = Cast<UProjectileAbility>(Spec.Ability))
		{
			Spec.GetDynamicSpecSourceTags().AddTag(ProjectileAbilityCDO->InputTag);

			ActiveProjectileAbility = GiveAbility(Spec);

			// Projectile tag goes on the per-actor instance created by GiveAbility.
			if (FGameplayAbilitySpec* GivenSpec = FindAbilitySpecFromHandle(ActiveProjectileAbility))
			{
				if (UProjectileAbility* ProjectileAbilityInstance = Cast<UProjectileAbility>(GivenSpec->GetPrimaryInstance()))
				{
					ProjectileAbilityInstance->SetProjectileToSpawn(ProjectileTag);
				}
			}
		}
	}
}

void URougeAbilitySystemComponent::ServerSetDynamicProjectile_Implementation(const FGameplayTag& ProjectileTag, int32 AbilityLevel)
{
	SetDynamicProjectile(ProjectileTag,AbilityLevel);
}
