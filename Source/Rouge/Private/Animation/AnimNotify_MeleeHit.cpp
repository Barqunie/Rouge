#include "Animation/AnimNotify_MeleeHit.h"

#include "AbilitySystem/RougeGameplayTag.h"
#include "MeleeAnimNotifyUtils.h"

void UAnimNotify_MeleeHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	RougeMeleeNotify::SendEventToOwner(MeshComp, RougeGameplayTags::Event::Melee_Hit);
}

FString UAnimNotify_MeleeHit::GetNotifyName_Implementation() const
{
	return TEXT("Melee Hit");
}
