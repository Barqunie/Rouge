// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Augments/RougeAugmentTypes.h"
#include "RougeMainGameMode.generated.h"

class UProjectileInfo;
class UCharacterClassInfo;
class URougeAugmentPool;
class ARougePlayerController;
class ARougePlayerState;

/**
 * 
 */
UCLASS()
class ROUGE_API ARougeMainGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ARougeMainGameMode();

	UCharacterClassInfo* GetCharacterClassDefaultInfo() const;

	
	UProjectileInfo* GetProjectileInfo() const;

	/** Spawns the character the player picked in the lobby (URougeCharacterSelectionSubsystem), else DefaultPawnClass. */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	/** Called by a player character when it dies. Fires OnAllPlayersDead when nobody is left alive. */
	void NotifyPlayerDied(class ARougeCharacter* DeadCharacter);

	/** Every player is dead at the same time (lose condition). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void OnAllPlayersDead();

	virtual void BeginPlay() override;
	virtual void Logout(AController* Exiting) override;

	// ---------------- Augments (level up cards) ----------------
	// On every team level up the game pauses and each player gets CardsPerOffer cards for their class.
	// The game resumes once everybody has picked (one round per level gained).

	/** A player picked card OfferIndex of their current offer (from ARougePlayerController::ServerChooseAugment). */
	void HandleAugmentChosen(ARougePlayerController* PlayerController, int32 OfferIndex);

	UFUNCTION(BlueprintPure, Category = "Augments")
	bool IsAugmentSelectionActive() const { return bAugmentRoundActive; }

protected:
	/** Card pool. None (or empty) uses the built-in default cards. */
	UPROPERTY(EditDefaultsOnly, Category = "Augments")
	TObjectPtr<URougeAugmentPool> AugmentPool;

	UPROPERTY(EditDefaultsOnly, Category = "Augments", meta = (ClampMin = "1", ClampMax = "4"))
	int32 CardsPerOffer = 3;

private:
	UFUNCTION()
	void HandleTeamLevelUp(int32 NewLevel);

	const TArray<FRougeAugmentDefinition>& GetAugments() const;
	const FRougeAugmentDefinition* FindAugment(FName AugmentId) const;

	/** Weighted random cards the player can still take (class + max stacks), no duplicates. */
	TArray<FName> RollOffers(const ARougePlayerController* PlayerController) const;
	FRougeAugmentOffer MakeOffer(const FRougeAugmentDefinition& Augment, const ARougePlayerState* PlayerState) const;

	/** Starts the next pending round (pauses the game). Skips rounds where nobody has a card left. */
	void StartAugmentRound();
	void FinishAugmentRound();
	void SetAugmentPause(bool bPaused);

	/** Rounds still owed (several levels can come at once). */
	int32 PendingAugmentRounds = 0;
	int32 LastHandledLevel = 1;
	bool bAugmentRoundActive = false;

	/** Players that still have to pick, with the card ids they were offered. */
	TMap<TWeakObjectPtr<ARougePlayerController>, TArray<FName>> PendingOffers;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;


	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectile Info")
	TObjectPtr<UProjectileInfo> ProjectileInfo;
};
