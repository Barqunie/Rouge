#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTagContainer.h"

namespace RougeMeleeNotify
{
	// Sends a gameplay event to the mesh owner's ASC. Silently ignored when there is no ASC (e.g. animation editor preview).
	inline void SendEventToOwner(USkeletalMeshComponent* MeshComp, const FGameplayTag& EventTag)
	{
		AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		if (!IsValid(Owner))
		{
			return;
		}

		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
		if (!IsValid(ASC))
		{
			return;
		}

		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Owner;

		ASC->HandleGameplayEvent(EventTag, &Payload);
	}
}
