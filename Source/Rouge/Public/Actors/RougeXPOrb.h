// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeXPOrb.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class ARougeCharacter;

/**
 * Dropped by enemies on death. Server only logic: drifts toward the nearest alive player inside MagnetRadius,
 * and when any alive player touches it the XP is added to the shared team XP (ARougeGameState).
 * Movement replicates so every client sees the orb fly in.
 */
UCLASS()
class ROUGE_API ARougeXPOrb : public AActor
{
	GENERATED_BODY()

public:
	ARougeXPOrb();

	virtual void Tick(float DeltaTime) override;

	/** Server: set by the enemy that dropped it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XP", meta = (ClampMin = "0.0"))
	float XPValue = 10.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "XP")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, Category = "XP")
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	/** Players within this distance pull the orb toward them. */
	UPROPERTY(EditDefaultsOnly, Category = "XP", meta = (ClampMin = "0.0"))
	float MagnetRadius = 350.f;

	/** Starting pull speed; it accelerates while being pulled. */
	UPROPERTY(EditDefaultsOnly, Category = "XP", meta = (ClampMin = "0.0"))
	float MagnetSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "XP", meta = (ClampMin = "0.0"))
	float MagnetAcceleration = 1500.f;

	/** Optional material; if it has a "Color" vector parameter, OrbColor is applied. */
	UPROPERTY(EditDefaultsOnly, Category = "XP")
	TObjectPtr<UMaterialInterface> OrbMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "XP")
	FLinearColor OrbColor = FLinearColor(0.25f, 1.f, 0.45f, 0.9f);

private:
	TWeakObjectPtr<ARougeCharacter> MagnetTarget;
	float NextMagnetSearchTime = 0.f;
	float CurrentPullSpeed = 0.f;
	bool bCollected = false;

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void TryCollect(AActor* OtherActor);
	ARougeCharacter* FindMagnetTarget() const;
};
