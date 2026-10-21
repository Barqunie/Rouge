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
class ARougePackage;

/** One offered card: which augment and where its amounts landed in their ranges (0..1). */
struct FRougeRolledAugment
{
	FName Id;
	float Roll = 1.f;
};

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

	/** The package reached the delivery zone (ARougeDeliveryZone). Wins the run. */
	void HandlePackageDelivered();

	/** Server: ends the run (stops spawning, shows the result, goes back to the lobby after ReturnToLobbyDelay). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
	void EndRun(bool bWon);

	/** Run ended (after the game state has the result). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void OnRunEnded(bool bWon);

	// ---------------- Waves ----------------
	// Each wave has a fixed number of enemies (spawned by the level's spawners). Killing all of them ends the
	// wave; after the last one the delivery zone opens.

	/** Spawners: how many more enemies may spawn right now (MAX_int32 without waves). */
	int32 GetWaveSpawnBudget() const;

	/** Spawners: one wave enemy was spawned. */
	void NotifyWaveEnemySpawned();

	/** A wave started (wave number from 1). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Waves")
	void OnWaveStarted(int32 Wave);

	/** Every wave is done, the delivery zone is open. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Waves")
	void OnAllWavesCleared();

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

	// ---------------- Waves ----------------

	/** 0 = no waves: spawners spawn freely and the delivery zone is open from the start. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0"))
	int32 NumberOfWaves = 10;

	/** Enemies in wave 1 for one player. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "1"))
	int32 FirstWaveEnemies = 20;

	/** Each later wave has this many more (one player). Player count multiplies the whole wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0"))
	int32 EnemiesAddedPerWave = 8;

	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0.0"))
	float TimeBeforeFirstWave = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0.0"))
	float TimeBetweenWaves = 8.f;

	// ---------------- Package run ----------------

	/** Spawned once at the start. None = no package (and no win condition). */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	TSubclassOf<ARougePackage> PackageClass;

	/** The package starts at an actor with this tag; without one, next to the first player start. */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	FName PackageStartTag = TEXT("PackageStart");

	/**
	 * Without an ARougeDeliveryZone placed in the level, one is spawned here (on the floor under it),
	 * so maps without a drop-off can still be played. Place a real one in the level to choose the spot.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	bool bSpawnDeliveryZoneIfMissing = true;

	UPROPERTY(EditDefaultsOnly, Category = "Package", meta = (EditCondition = "bSpawnDeliveryZoneIfMissing"))
	FVector FallbackDeliveryLocation = FVector(-1300.f, 0.f, 0.f);

	/** Seconds the result banner stays before going back to the lobby. */
	UPROPERTY(EditDefaultsOnly, Category = "Package", meta = (ClampMin = "0.0"))
	float ReturnToLobbyDelay = 10.f;

	/** Map to travel to after the run. Empty = stay. */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	FString LobbyMap = TEXT("/Game/Maps/Lobby");

private:
	UFUNCTION()
	void HandleTeamLevelUp(int32 NewLevel);

	const TArray<FRougeAugmentDefinition>& GetAugments() const;
	const FRougeAugmentDefinition* FindAugment(FName AugmentId) const;

	/** Weighted random cards the player can still take (class + max stacks), no duplicates, each with its own roll. */
	TArray<FRougeRolledAugment> RollOffers(const ARougePlayerController* PlayerController) const;
	FRougeAugmentOffer MakeOffer(const FRougeAugmentDefinition& Augment, float Roll, const ARougePlayerState* PlayerState) const;

	/** Starts the next pending round (pauses the game). Skips rounds where nobody has a card left. */
	void StartAugmentRound();
	void FinishAugmentRound();
	void SetAugmentPause(bool bPaused);

	/** Rounds still owed (several levels can come at once). */
	int32 PendingAugmentRounds = 0;
	int32 LastHandledLevel = 1;
	bool bAugmentRoundActive = false;

	/** Players that still have to pick, with the card ids they were offered. */
	TMap<TWeakObjectPtr<ARougePlayerController>, TArray<FRougeRolledAugment>> PendingOffers;

	void SpawnPackage();
	void ReturnToLobby();

	void StartBreak(float Duration);
	void StartNextWave();
	/** 4x per second: counts alive enemies, ends the wave / break, pushes the numbers to the game state. */
	void UpdateWaves();
	int32 CountAliveEnemies() const;

	int32 CurrentWaveNumber = 0;
	int32 WaveEnemiesToSpawn = 0;
	bool bWaveFighting = false;
	bool bWavesCleared = false;
	float BreakEndTime = 0.f;
	FTimerHandle WaveUpdateTimer;

	FTimerHandle ReturnToLobbyTimer;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;


	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectile Info")
	TObjectPtr<UProjectileInfo> ProjectileInfo;
};
