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
class ARougeStation;

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

	/** Server: ends the run (stops spawning, shows the result, goes back to the lobby after ReturnToLobbyDelay). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
	void EndRun(bool bWon);

	/** Run ended (after the game state has the result). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void OnRunEnded(bool bWon);

	// ---------------- Stations & waves ----------------
	// A random unpurified station is chosen; bringing the package there docks it, starts that station's wave and
	// fills the purification bar while it stays docked. Bar full + wave cleared = station done. After every
	// station the final one is chosen; finishing it wins the run.

	/** Spawners: how many more enemies may spawn right now (MAX_int32 without waves). */
	int32 GetWaveSpawnBudget() const;

	/** Spawners: one wave enemy was spawned. */
	void NotifyWaveEnemySpawned();

	/** Test command: the active station is finished right away (its enemies are removed). */
	void DebugCompleteActiveStation();

	/** Spawners: only the ones near the active station spawn (all of them if none is near). */
	bool ShouldSpawnerSpawn(const AActor* Spawner) const;

	/** A station's wave started (stage from 1). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stations")
	void OnWaveStarted(int32 Stage);

	/** A (non final) station was purified. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stations")
	void OnStationPurified(int32 Stage);

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

	// ---------------- Stations & waves ----------------

	/** Enemies in the first station's wave for one player. */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "1"))
	int32 FirstWaveEnemies = 30;

	/** Each later station's wave has this many more (one player). Player count multiplies the whole wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "0"))
	int32 EnemiesAddedPerWave = 15;

	/** The final wave is the next step of the curve times this. */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "1.0"))
	float FinalWaveMultiplier = 1.3f;

	/** Spawners within this distance of the active station spawn its wave (all of them if none is that close). */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "500.0"))
	float StationSpawnerRange = 4000.f;

	/** Without ARougeStation actors in the level, these are spawned (three stations + the final) for testing. */
	UPROPERTY(EditDefaultsOnly, Category = "Stations")
	bool bSpawnStationsIfMissing = true;

	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (EditCondition = "bSpawnStationsIfMissing"))
	TArray<FVector> FallbackStationLocations = { FVector(-1300.f, 1100.f, 0.f), FVector(1300.f, 1100.f, 0.f), FVector(1300.f, -1100.f, 0.f) };

	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (EditCondition = "bSpawnStationsIfMissing"))
	FVector FallbackFinalLocation = FVector(-1300.f, -1100.f, 0.f);

	// ---------------- Package run ----------------

	/** Spawned once at the start. None = no package (and no win condition). */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	TSubclassOf<ARougePackage> PackageClass;

	/** The package starts at an actor with this tag; without one, next to the first player start. */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	FName PackageStartTag = TEXT("PackageStart");

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

	/** Finds (or spawns the fallback) stations and numbers them. */
	void SetupStations();
	/** Picks a random unpurified station, or the final one when they are all done. */
	void ActivateNextStation();
	/** 10x per second: docking, purification, wave progress, stage completion, game state numbers. */
	void UpdateStations();
	void StartStageWave();
	void CompleteStage();
	int32 CountAliveEnemies() const;
	int32 GetTotalStages() const;

	TArray<TWeakObjectPtr<ARougeStation>> Stations;
	TWeakObjectPtr<ARougeStation> FinalStation;
	TWeakObjectPtr<ARougeStation> ActiveStation;
	/** Active station has no spawner in range: every spawner helps. */
	bool bAllSpawnersForStage = true;
	int32 StageNumber = 0;
	int32 WaveEnemiesToSpawn = 0;
	bool bStageWaveStarted = false;
	bool bAllStagesDone = false;
	float LastStationUpdateTime = 0.f;
	FTimerHandle StationUpdateTimer;

	FTimerHandle ReturnToLobbyTimer;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;


	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectile Info")
	TObjectPtr<UProjectileInfo> ProjectileInfo;
};
