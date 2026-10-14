// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "RougeGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeamXPChanged, float, CurrentXP, float, XPToNextLevel, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamLevelUp, int32, NewLevel);

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

protected:
	/** XP needed for level 1 -> 2. */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "1.0"))
	float BaseXPToLevel = 100.f;

	/** Extra XP needed per level after that (linear growth). */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "0.0"))
	float XPGrowthPerLevel = 50.f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_TeamProgress)
	float TeamXP = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_TeamProgress)
	int32 TeamLevel = 1;

	/** Last level this machine announced, so clients can detect level ups from replication. */
	int32 LastBroadcastLevel = 1;

	UFUNCTION()
	void OnRep_TeamProgress();

	void HandleLevelChange();
};
