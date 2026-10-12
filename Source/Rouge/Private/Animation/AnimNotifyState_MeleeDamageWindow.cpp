#include "Animation/AnimNotifyState_MeleeDamageWindow.h"

#include "AbilitySystem/RougeGameplayTag.h"
#include "MeleeAnimNotifyUtils.h"

void UAnimNotifyState_MeleeDamageWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	RougeMeleeNotify::SendEventToOwner(MeshComp, RougeGameplayTags::Event::Melee_WindowOpen);
}

void UAnimNotifyState_MeleeDamageWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	RougeMeleeNotify::SendEventToOwner(MeshComp, RougeGameplayTags::Event::Melee_WindowClose);

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UAnimNotifyState_MeleeDamageWindow::GetNotifyName_Implementation() const
{
	return TEXT("Melee Damage Window");
}
