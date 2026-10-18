// The list of augment cards that can be offered on level up.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Augments/RougeAugmentTypes.h"
#include "RougeAugmentPool.generated.h"

/**
 * Card pool. Assign one on the game mode (AugmentPool); without one (or with an empty list) the built-in
 * default cards are used. "Fill With Defaults" copies the built-in cards into the asset to edit them.
 */
UCLASS(BlueprintType)
class ROUGE_API URougeAugmentPool : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augments", meta = (TitleProperty = "Id"))
	TArray<FRougeAugmentDefinition> Augments;

	/** Replaces Augments with the built-in default cards. */
	UFUNCTION(CallInEditor, Category = "Augments")
	void FillWithDefaults();

	/** Built-in cards (stat, per-class skill and passive cards). */
	static const TArray<FRougeAugmentDefinition>& GetDefaultAugments();
};

namespace RougeAugments
{
	/** Amount for a roll in 0..1 (Min..Max), rounded the way the card shows it. */
	ROUGE_API float ResolveAmount(float Min, float Max, ERougeAugmentValueFormat Format, float Roll);

	/** Card text with {0} replaced by the rolled amount of the first mod. */
	ROUGE_API FText FormatDescription(const FRougeAugmentDefinition& Augment, float Roll);
}
