// Lobby character select screen (built in code like the HUD): portrait row + info about the hovered / picked character.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RougeCharacterSelectWidget.generated.h"

class ARougeCharacter;
class ARougeLobbyPlayerController;
class UButton;
class UHorizontalBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
class URougeCharacterSelectWidget;
struct FRougeCharacterDisplayInfo;

/** Forwards one portrait button's events with its index (dynamic delegates carry no payload). */
UCLASS()
class URougeCharacterCardHandler : public UObject
{
	GENERATED_BODY()

public:
	int32 Index = INDEX_NONE;
	TWeakObjectPtr<URougeCharacterSelectWidget> Owner;

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();
};

/**
 * Valorant-like picker: a row of character portraits at the bottom, details (role, description, abilities) on the left.
 * Everything shown comes from each character Blueprint's DisplayInfo. Clicking a portrait picks it
 * (ARougeLobbyPlayerController::SelectCharacter).
 */
UCLASS()
class ROUGE_API URougeCharacterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void OnCardClicked(int32 Index);
	void OnCardHovered(int32 Index);
	void OnCardUnhovered(int32 Index);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Layout (a Blueprint child of this widget, set on BP_LobbyPC, can move things to fit the lobby camera).

	/** Portrait row: distance from the bottom of the screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Layout")
	float RosterBottomOffset = 110.f;

	/** Info panel: offset from the middle of the left edge. */
	UPROPERTY(EditDefaultsOnly, Category = "Layout")
	FVector2D InfoPanelOffset = FVector2D(40.f, -30.f);

	UPROPERTY(EditDefaultsOnly, Category = "Layout")
	float TitleTopOffset = 28.f;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLayout();
	void RebuildRoster();
	void ShowInfo(int32 Index);
	void RefreshCardStyles();

	ARougeLobbyPlayerController* GetLobbyController() const;
	const ARougeCharacter* GetCharacterDefaults(int32 Index) const;
	FText GetCharacterName(int32 Index) const;

	UTextBlock* MakeText(const FText& Text, int32 Size, const FLinearColor& Color, bool bBold = true);
	/** Rounded picture, or a colored box with a letter when there is no texture. */
	UWidget* MakePicture(UTexture2D* Texture, const FLinearColor& Color, const FText& Letter, float Size, int32 LetterSize);

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> RosterRow;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> InfoPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RoleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> AbilityList;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> CardButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URougeCharacterCardHandler>> CardHandlers;

	UPROPERTY(Transient)
	TArray<TSubclassOf<ARougeCharacter>> Roster;

	int32 HoveredIndex = INDEX_NONE;
	int32 ShownIndex = INDEX_NONE;
	int32 StyledSelection = INDEX_NONE - 1;
	float RosterCheckCountdown = 0.f;
};
