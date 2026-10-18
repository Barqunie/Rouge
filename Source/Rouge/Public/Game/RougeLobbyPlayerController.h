// Lobby player controller: owns the character select screen and sends the pick to the server.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RougeLobbyPlayerController.generated.h"

class ARougeCharacter;
class URougeCharacterSelectWidget;

/**
 * Parent of BP_LobbyPC. The local player gets the character select screen (portraits + abilities).
 * Clicking a portrait calls ServerSelectCharacter; the server applies it through the lobby game mode
 * (SelectCharByIndex: platform preview + URougeCharacterSelectionSubsystem), see ApplyCharacterSelection.
 */
UCLASS()
class ROUGE_API ARougeLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARougeLobbyPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Characters in the picker, in game mode index order. */
	UFUNCTION(BlueprintPure, Category = "Character Select")
	TArray<TSubclassOf<ARougeCharacter>> GetSelectableCharacters() const;

	/** Local: pick a character (index into GetSelectableCharacters). */
	UFUNCTION(BlueprintCallable, Category = "Character Select")
	void SelectCharacter(int32 Index);

	/** The pick (set right away locally, confirmed by the server). */
	UFUNCTION(BlueprintPure, Category = "Character Select")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintCallable, Category = "Character Select")
	void SetCharacterSelectVisible(bool bVisible);

	UFUNCTION(Server, Reliable)
	void ServerSelectCharacter(int32 Index);

protected:
	virtual void BeginPlay() override;

	/**
	 * Server: apply the pick. Default calls the game mode's GameModeSelectFunction(this, Index)
	 * (BP_LobbyGameMode::SelectCharByIndex), or only stores the class in the selection subsystem if it has none.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Character Select")
	void ApplyCharacterSelection(int32 Index);

	/** Picker list. Empty = read the lobby game mode's GameModeCharacterListProperty, so there is one list to edit. */
	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	TArray<TSubclassOf<ARougeCharacter>> SelectableCharacters;

	/** Character class array on the lobby game mode Blueprint. */
	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	FName GameModeCharacterListProperty = TEXT("AvailableCharacterClasses");

	/** Game mode function (Controller, Index) that applies a pick. */
	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	FName GameModeSelectFunction = TEXT("SelectCharByIndex");

	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	TSubclassOf<URougeCharacterSelectWidget> CharacterSelectWidgetClass;

	/** Under the lobby UI so its loading screen covers the picker. */
	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	int32 CharacterSelectZOrder = -1;

private:
	UPROPERTY(Replicated)
	int32 SelectedIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<URougeCharacterSelectWidget> CharacterSelectWidget;

	bool CallGameModeSelectFunction(int32 Index);
};
