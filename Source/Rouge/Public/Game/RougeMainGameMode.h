// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "RougeMainGameMode.generated.h"

class UProjectileInfo;
class UCharacterClassInfo;

/**
 * 
 */
UCLASS()
class ROUGE_API ARougeMainGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	UCharacterClassInfo* GetCharacterClassDefaultInfo() const;

	
	UProjectileInfo* GetProjectileInfo() const;

	/** Spawns the character the player picked in the lobby (URougeCharacterSelectionSubsystem), else DefaultPawnClass. */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	/** Called by a player character when it dies. Fires OnAllPlayersDead when nobody is left alive. */
	void NotifyPlayerDied(class ARougeCharacter* DeadCharacter);

	/** Every player is dead at the same time (lose condition). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void OnAllPlayersDead();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;


	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectile Info")
	TObjectPtr<UProjectileInfo> ProjectileInfo;
};
