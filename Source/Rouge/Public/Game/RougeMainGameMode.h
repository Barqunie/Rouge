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

	// ---------------- Waves & stations ----------------
	// Waves come one after the other (short break between them), each a bit bigger. Every few waves (random,
	// MinWavesBetweenStations..Max) a random unpurified station opens as a side objective: bring the package,
	// it docks and purifies while it stays there (enemies try to steal it). A purified station gives the team
	// a level. After every station the final one opens: docking there starts the big final wave, purifying it and
	// clearing that wave wins the run.

	/** Spawners: how many more enemies may spawn right now (MAX_int32 without waves). */
	int32 GetWaveSpawnBudget() const;

	/** Spawners: one wave enemy was spawned. */
	void NotifyWaveEnemySpawned();

	/** Test command: opens the next station now, or (when one is open) finishes it right away. */
	void DebugCompleteActiveStation();

	/** Spawners: while the package purifies, only the ones near that station spawn (all of them if none is near). */
	bool ShouldSpawnerSpawn(const AActor* Spawner) const;

	/** A wave started (from 1). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Waves")
	void OnWaveStarted(int32 Wave);

	/** A station opened (StationNumber 0 = the final). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stations")
	void OnStationOpened(int32 StationNumber);

	/** A (non final) station was purified. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stations")
	void OnStationPurified(int32 StationNumber);

	/** The package was docked at the final station: the final wave is coming. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stations")
	void OnFinalStarted();

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

	/** Dev only (set on the running game mode): level ups don't pause for cards, so tools can test long runs. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugSkipAugmentRounds = false;

	// ---------------- Waves & stations ----------------

	/** Enemies in wave 1 for one player. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "1"))
	int32 FirstWaveEnemies = 20;

	/** Each later wave has this many more (one player). Player count multiplies the whole wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0"))
	int32 EnemiesAddedPerWave = 7;

	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0.0"))
	float TimeBeforeFirstWave = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0.0"))
	float TimeBetweenWaves = 7.f;

	/** A station opens after a random number of cleared waves in this range (counted again after each station). */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "1"))
	int32 MinWavesBetweenStations = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "1"))
	int32 MaxWavesBetweenStations = 3;

	/** Purifying a station gives the team this many levels worth of XP (1 = a level up, so a card pick). */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "0.0"))
	float StationRewardLevels = 1.f;

	/** The final wave is the next wave of the curve times this. */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "1.0"))
	float FinalWaveMultiplier = 1.6f;

	/** Spawners within this distance of the active station spawn its wave (all of them if none is that close). */
	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (ClampMin = "500.0"))
	float StationSpawnerRange = 4000.f;

	/** Without ARougeStation actors in the level, these are spawned (three stations + the final) for testing. */
	UPROPERTY(EditDefaultsOnly, Category = "Stations")
	bool bSpawnStationsIfMissing = true;

	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (EditCondition = "bSpawnStationsIfMissing"))
	TArray<FVector> FallbackStationLocations = { FVector(-2200.f, 2000.f, 0.f), FVector(2300.f, 1700.f, 0.f), FVector(1900.f, -2200.f, 0.f) };

	UPROPERTY(EditDefaultsOnly, Category = "Stations", meta = (EditCondition = "bSpawnStationsIfMissing"))
	FVector FallbackFinalLocation = FVector(-1800.f, -1700.f, 0.f);

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
	/** Opens a random unpurified station, or the final one when they are all done. */
	void ActivateNextStation();
	/** 10x per second: waves, docking, purification, station completion, game state numbers. */
	void UpdateRun();
	void UpdateActiveStation(float DeltaTime);
	void StartBreak(float Duration);
	void StartNextWave();
	void StartFinalWave();
	void CompleteStation();
	void PushRunState();
	int32 CountAliveEnemies() const;
	int32 CountPurifiedStations() const;
	int32 RollWavesUntilStation() const;
	int32 GetPlayerScaledWaveSize(float BaseEnemies) const;

	TArray<TWeakObjectPtr<ARougeStation>> Stations;
	TWeakObjectPtr<ARougeStation> FinalStation;
	TWeakObjectPtr<ARougeStation> ActiveStation;
	/** Active station has no spawner in range: every spawner helps. */
	bool bAllSpawnersForStation = true;

	int32 WaveNumber = 0;
	int32 WaveEnemiesToSpawn = 0;
	bool bWaveFighting = false;
	float BreakEndTime = 0.f;
	float BreakEndServerTime = 0.f;
	/** Cleared waves still needed before the next station opens. */
	int32 WavesUntilStation = 0;
	bool bFinalStarted = false;
	bool bRunWon = false;
	float LastUpdateTime = 0.f;
	FTimerHandle RunUpdateTimer;

	FTimerHandle ReturnToLobbyTimer;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;


	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectile Info")
	TObjectPtr<UProjectileInfo> ProjectileInfo;
};
