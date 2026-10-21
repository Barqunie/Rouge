// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "RougeGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeamXPChanged, float, CurrentXP, float, XPToNextLevel, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamLevelUp, int32, NewLevel);

UENUM(BlueprintType)
enum class ERougeRunResult : uint8
{
	InProgress,
	/** The package reached the delivery zone. */
	Won,
	/** Every player was dead at the same time. */
	Lost
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunResultChanged, ERougeRunResult, Result);

/**
 * Shared team progression (LoL Swarm style): one XP bar and level for the whole team.
 * Server adds XP; values replicate to everyone. Local player characters get OnXPChanged so the HUD XP bar updates.
 */
UCLASS()
class ROUGE_API ARougeGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server only. Adds XP to the team, handling any number of level ups. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Team XP")
	void AddTeamXP(float Amount);

	UFUNCTION(BlueprintPure, Category = "Team XP")
	float GetTeamXP() const { return TeamXP; }

	UFUNCTION(BlueprintPure, Category = "Team XP")
	int32 GetTeamLevel() const { return TeamLevel; }

	UFUNCTION(BlueprintPure, Category = "Team XP")
	float GetXPToNextLevel() const;

	/** Fires on every machine whenever team XP or level changes. */
	UPROPERTY(BlueprintAssignable, Category = "Team XP")
	FOnTeamXPChanged OnTeamXPChanged;

	/** Fires on every machine when the team levels up (augment card selection hooks in here). */
	UPROPERTY(BlueprintAssignable, Category = "Team XP")
	FOnTeamLevelUp OnTeamLevelUp;

	/** Pushes the current values to delegates and local player characters (HUD XP bar). */
	void BroadcastTeamXP();

	// ---------------- Run ----------------

	UFUNCTION(BlueprintPure, Category = "Run")
	ERougeRunResult GetRunResult() const { return RunResult; }

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRunOver() const { return RunResult != ERougeRunResult::InProgress; }

	/** Server only (game mode). */
	void SetRunResult(ERougeRunResult NewResult);

	/** Fires on every machine when the run is won or lost. */
	UPROPERTY(BlueprintAssignable, Category = "Run")
	FOnRunResultChanged OnRunResultChanged;

	// ---------------- Difficulty ----------------
	// Enemies get tougher with the team level and with every extra player, spawns speed up with the level.
	// Read by enemies when they spawn and by the spawners for each batch (server).

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	int32 GetPlayerCount() const;

	/** Applied to an enemy's max health when it spawns. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyHealthMultiplier() const;

	/** Applied to an enemy's attack damage when it spawns. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyDamageMultiplier() const;

	/** Spawn intervals are divided by this. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetSpawnRateMultiplier() const;

	/** Spawner alive caps are multiplied by this. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyCapMultiplier() const;

	/** Player count part of the enemy count: spawn rate and alive cap are both multiplied by this. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyCountMultiplier() const;

	/** Hard limit on alive enemies (performance), whatever the multipliers say. */
	int32 GetMaxAliveEnemiesTotal() const { return MaxAliveEnemiesTotal; }

protected:
	/** XP needed for level 1 -> 2. */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "1.0"))
	float BaseXPToLevel = 100.f;

	/** Extra XP needed per level after that (linear growth). */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "0.0"))
	float XPGrowthPerLevel = 50.f;

	/**
	 * Each extra player raises the XP needed per level by this fraction. Kept equal to EnemyCountPerExtraPlayer:
	 * N players get that many more enemies (= XP), so levels come at the same pace for any team size.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "0.0"))
	float XPPerExtraPlayer = 0.6f;

	// Balance (per player count P and team level L):
	//   enemy count  x (1 + 0.6 (P-1))       1p 1.0, 2p 1.6, 3p 2.2, 4p 2.8
	//   enemy health x (1 + 0.15 (P-1))      1p 1.0, 2p 1.15, 3p 1.3, 4p 1.45
	//   => total enemy HP on the map grows ~P (1.0 / 1.84 / 2.86 / 4.06), so each player has about the same work.
	//   enemy health x (1 + 0.12 (L-1)), damage x (1 + 0.06 (L-1)): a bit faster than cards make players stronger,
	//   so later levels stay tense (melee enemy 30 HP at L1, 62 at L10, 80 at L15 solo).

	/** Each extra player adds this fraction to the spawn rate and the alive cap. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyCountPerExtraPlayer = 0.6f;

	/** Enemy health +this fraction per team level above 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyHealthPerLevel = 0.12f;

	/** Enemy health +this fraction per player above 1 (on top of more enemies). */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyHealthPerExtraPlayer = 0.15f;

	/** Enemy damage +this fraction per team level above 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyDamagePerLevel = 0.06f;

	/** Spawn rate +this fraction per team level above 1, up to MaxSpawnRateMultiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float SpawnRatePerLevel = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "1.0"))
	float MaxSpawnRateMultiplier = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "1"))
	int32 MaxAliveEnemiesTotal = 80;

	/** Alive cap +this fraction per team level above 1, up to MaxEnemyCapMultiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyCapPerLevel = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "1.0"))
	float MaxEnemyCapMultiplier = 2.5f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_TeamProgress)
	float TeamXP = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_TeamProgress)
	int32 TeamLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_RunResult)
	ERougeRunResult RunResult = ERougeRunResult::InProgress;

	UFUNCTION()
	void OnRep_RunResult();

	/** Last level this machine announced, so clients can detect level ups from replication. */
	int32 LastBroadcastLevel = 1;

	UFUNCTION()
	void OnRep_TeamProgress();

	void HandleLevelChange();
};
