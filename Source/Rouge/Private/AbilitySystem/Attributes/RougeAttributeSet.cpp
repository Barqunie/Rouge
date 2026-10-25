// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "GameplayEffectExtension.h"
#include <Net/UnrealNetwork.h>
#include <AbilitySystem/RougeAbilityTypes.h>
#include "Character/CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/RougeGameplayTag.h"
#include "RougePlayerController.h"
#include "Character/EnemyBase.h"

void URougeAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Health
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	//Critical Strike
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, CritDamage, COND_None, REPNOTIFY_Always);

	//Combat
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Damage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, DamageReduction, COND_None, REPNOTIFY_Always);

	//Experience Points
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, XP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, MaxXP, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, LifeSteal, COND_None, REPNOTIFY_Always);
	
	
}

void URougeAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if(Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}


	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Data);
	}

}

void URougeAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{	
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Health, OldHealth);
}

void URougeAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, MaxHealth, OldMaxHealth);
}




void URougeAttributeSet::OnRep_Damage(const FGameplayAttributeData& OldDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Damage, OldDamage);
}

void URougeAttributeSet::OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, DamageReduction, OldDamageReduction);
}

void URougeAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Armor, OldArmor);
}

void URougeAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, AttackSpeed, OldAttackSpeed);
}




void URougeAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldCritChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, CritChance, OldCritChance);
}
void URougeAttributeSet::OnRep_CritDamage(const FGameplayAttributeData& OldCritDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, CritDamage, OldCritDamage);
}




void URougeAttributeSet::OnRep_XP(const FGameplayAttributeData& OldXP)
{		
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, XP, OldXP);

}

void URougeAttributeSet::OnRep_MaxXP(const FGameplayAttributeData& OldMaxXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, MaxXP, OldMaxXP);



}

void URougeAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Shield, OldShield);
}

void URougeAttributeSet::OnRep_LifeSteal(const FGameplayAttributeData& OldLifeSteal)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, LifeSteal, OldLifeSteal);
}

void URougeAttributeSet::ApplyLifeSteal(const FGameplayEffectModCallbackData& Data, float DamageDealt)
{
	UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!SourceASC || DamageDealt <= 0.f || SourceASC == &Data.Target)
	{
		return;
	}

	const URougeAttributeSet* SourceSet = SourceASC->GetSet<URougeAttributeSet>();
	if (!SourceSet || SourceSet->GetLifeSteal() <= 0.f || SourceASC->HasMatchingGameplayTag(RougeGameplayTags::State::Dead))
	{
		return;
	}

	// Capped so stacking cards can't make a player unkillable.
	const float Percent = FMath::Min(SourceSet->GetLifeSteal(), 30.f);
	const float Heal = FMath::Min(DamageDealt * Percent / 100.f, SourceSet->GetMaxHealth() - SourceSet->GetHealth());
	if (Heal > 0.f)
	{
		SourceASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, Heal);
	}
}

void URougeAttributeSet::ShowDamageNumbers(const FGameplayEffectModCallbackData& Data, float DamageDealt, bool bCriticalHit) const
{
	const int32 Amount = FMath::RoundToInt(DamageDealt);
	AActor* TargetActor = Data.Target.AbilityActorInfo.IsValid() ? Data.Target.AbilityActorInfo->AvatarActor.Get() : nullptr;
	if (Amount <= 0 || !TargetActor)
	{
		return;
	}

	// Just above the head.
	const FVector Location = TargetActor->GetActorLocation() + FVector(0.f, 0.f, TargetActor->GetSimpleCollisionHalfHeight() + 15.f);

	// Knockback away from whoever hit it (enemies only; crits push harder).
	if (AEnemyBase* Enemy = Cast<AEnemyBase>(TargetActor))
	{
		const UAbilitySystemComponent* KnockSourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
		const AActor* SourceAvatar = KnockSourceASC ? KnockSourceASC->GetAvatarActor() : nullptr;
		if (SourceAvatar && SourceAvatar != Enemy)
		{
			// Normal hit: small hop back (~55 units), crit: real pop (~200 units).
			Enemy->ApplyKnockback(Enemy->GetActorLocation() - SourceAvatar->GetActorLocation(),
				bCriticalHit ? 600.f : 300.f, bCriticalHit ? 160.f : 90.f);
		}
	}

	// Chempunk splash on the body (replicated by the ASC to everyone). Crit = strong version.
	if (UAbilitySystemComponent* TargetASC = Data.Target.AbilityActorInfo->AbilitySystemComponent.Get())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = TargetActor->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		CueParameters.NormalizedMagnitude = bCriticalHit ? 1.f : 0.f;
		TargetASC->ExecuteGameplayCue(RougeGameplayTags::Cue::Chem_Hit, CueParameters);
	}

	ARougePlayerController* TargetController = Cast<ARougePlayerController>(Data.Target.AbilityActorInfo->PlayerController.Get());

	const UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	ARougePlayerController* SourceController = SourceASC && SourceASC->AbilityActorInfo.IsValid()
		? Cast<ARougePlayerController>(SourceASC->AbilityActorInfo->PlayerController.Get())
		: nullptr;

	if (SourceController && SourceController != TargetController)
	{
		SourceController->ClientShowDamageNumber(Location, Amount, bCriticalHit ? ERougeDamageNumberType::Critical : ERougeDamageNumberType::Dealt);
	}
	if (TargetController)
	{
		TargetController->ClientShowDamageNumber(Location, Amount, ERougeDamageNumberType::Taken);
	}
}

void URougeAttributeSet::HandleIncomingDamage(const FGameplayEffectModCallbackData& Data)
{
	const float localDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	FGameplayEffectContextHandle ContextHandle = Data.EffectSpec.GetContext();
	FRougeGameplayEffectContext* RougeContext = FRougeGameplayEffectContext::GetEffectContext(ContextHandle);

	const bool bCriticalHit = RougeContext && RougeContext->IsCriticalHit();
	ShowDamageNumbers(Data, localDamage, bCriticalHit);

	// Life steal counts damage that actually landed (shield + health), not overkill.
	ApplyLifeSteal(Data, FMath::Min(localDamage, GetShield() + GetHealth()));

	// Shield absorbs first.
	float RemainingDamage = localDamage;
	const float CurrentShield = GetShield();
	if (CurrentShield > 0.f && RemainingDamage > 0.f)
	{
		const float Absorbed = FMath::Min(CurrentShield, RemainingDamage);
		SetShield(CurrentShield - Absorbed);
		RemainingDamage -= Absorbed;
	}

	SetHealth(FMath::Clamp(GetHealth() - RemainingDamage, 0.f, GetMaxHealth()));

	// Server: let the character handle its own death (player: downed + revive, enemy: destroy).
	if (GetHealth() <= 0.f && Data.Target.AbilityActorInfo.IsValid())
	{
		if (ACharacterBase* Character = Cast<ACharacterBase>(Data.Target.AbilityActorInfo->AvatarActor.Get()))
		{
			if (!Character->IsDead())
			{
				Character->HandleHealthDepleted();
			}
		}
	}
}
