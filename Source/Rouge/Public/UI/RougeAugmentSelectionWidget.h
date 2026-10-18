// Level up card picker (built in code like the HUD).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Augments/RougeAugmentTypes.h"
#include "RougeAugmentSelectionWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UTextBlock;

/**
 * Full screen card picker shown while the game is paused for a level up.
 * Clicking a card sends the pick to the server (ARougePlayerController::ChooseAugment); the widget then waits
 * for teammates until the controller closes it.
 */
UCLASS()
class ROUGE_API URougeAugmentSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Replaces the cards (at most 4). */
	void ShowOffers(const TArray<FRougeAugmentOffer>& InOffers, int32 TeamLevel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	UButton* BuildCard(const FRougeAugmentOffer& Offer);
	FLinearColor GetCategoryColor(ERougeAugmentCategory Category) const;
	FText GetCategoryLabel(ERougeAugmentCategory Category) const;
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = true);

	void Choose(int32 Index);

	// Dynamic delegates can't carry the card index, so one handler per card slot.
	UFUNCTION()
	void HandleCard0Clicked();

	UFUNCTION()
	void HandleCard1Clicked();

	UFUNCTION()
	void HandleCard2Clicked();

	UFUNCTION()
	void HandleCard3Clicked();

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> CardRow;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> CardButtons;

	TArray<FRougeAugmentOffer> Offers;
	bool bChosen = false;
};
