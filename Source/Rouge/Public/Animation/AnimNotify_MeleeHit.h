#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_MeleeHit.generated.h"

/**
 * Place on a melee montage at the impact frame.
 * Sends Event.Melee.Hit to the owner's ASC; UMeleeDamageAbility does a single instant hit check.
 */
UCLASS(meta = (DisplayName = "Melee Hit"))
class ROUGE_API UAnimNotify_MeleeHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
