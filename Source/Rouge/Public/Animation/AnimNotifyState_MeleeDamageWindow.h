#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MeleeDamageWindow.generated.h"

/**
 * Place on a melee montage over the frames where the weapon should deal damage.
 * Sends Event.Melee.WindowOpen / Event.Melee.WindowClose to the owner's ASC; UMeleeDamageAbility opens/closes its hit window.
 */
UCLASS(meta = (DisplayName = "Melee Damage Window"))
class ROUGE_API UAnimNotifyState_MeleeDamageWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
