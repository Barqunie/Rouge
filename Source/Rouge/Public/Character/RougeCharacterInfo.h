// What the character select screen and the HUD show about a playable character (set on each character Blueprint).

#pragma once

#include "CoreMinimal.h"
#include "RougeCharacterInfo.generated.h"

class UTexture2D;

/** One ability row (basic attack, Q, E, passive...). */
USTRUCT(BlueprintType)
struct FRougeAbilityDisplayInfo
{
	GENERATED_BODY()

	/** Key label: "Q", "E", "LMB"... Q / E rows also feed the HUD skill slots when the ability has no icon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select")
	FText Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select", meta = (MultiLine = true))
	FText Description;

	/** Without one, a colored box with the key is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select")
	TObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct FRougeCharacterDisplayInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select")
	FText Name;

	/** Short role line: "Melee damage", "Support"... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select")
	FText Role;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select", meta = (MultiLine = true))
	FText Description;

	/** Square picture for the select screen and the HUD portrait. Without one, a colored box with the initial is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select")
	TObjectPtr<UTexture2D> Portrait;

	/** Accent color (frames, role text, HUD portrait). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select")
	FLinearColor Color = FLinearColor(0.85f, 0.25f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Select", meta = (TitleProperty = "Name"))
	TArray<FRougeAbilityDisplayInfo> Abilities;

	const FRougeAbilityDisplayInfo* FindAbility(const FString& Key) const
	{
		return Abilities.FindByPredicate([&Key](const FRougeAbilityDisplayInfo& Ability)
		{
			return Ability.Key.ToString().Equals(Key, ESearchCase::IgnoreCase);
		});
	}
};
