// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeAreaIndicator.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UNiagaraSystem;

/**
 * Short-lived placeholder effect for skills: a ground circle that expands and fades, an optional light pillar flash
 * and an optional Niagara burst. Spawned on the server with SpawnIndicator; replicates to clients, which play the
 * cosmetic parts locally.
 */
UCLASS()
class ROUGE_API ARougeAreaIndicator : public AActor
{
	GENERATED_BODY()

public:
	ARougeAreaIndicator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Server only. Places the effect on the ground under Location.
	 * bBurst: expanding ring + pillar flash (impact); otherwise a steady circle (telegraph / stun marker).
	 */
	static ARougeAreaIndicator* SpawnIndicator(UWorld* World, const FVector& Location, float InRadius, const FLinearColor& InColor,
		float InDuration, bool bBurst = false, UNiagaraSystem* InVFX = nullptr, float InVFXScale = 1.f);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Indicator")
	TObjectPtr<UStaticMeshComponent> CircleMesh;

	UPROPERTY(VisibleAnywhere, Category = "Indicator")
	TObjectPtr<UStaticMeshComponent> PillarMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	float Radius = 200.f;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	FLinearColor Color = FLinearColor(1.f, 0.5f, 0.1f, 0.5f);

	UPROPERTY(Replicated)
	float Duration = 0.5f;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	bool bBurstEffect = false;

	UPROPERTY(Replicated)
	TObjectPtr<UNiagaraSystem> BurstVFX;

	UPROPERTY(Replicated)
	float VFXScale = 1.f;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CircleMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PillarMID;

	float Age = 0.f;
	bool bVFXSpawned = false;

	UFUNCTION()
	void OnRep_Visuals();

	void ApplyVisuals();
	void SetCircleRadius(float CurrentRadius);
	void TrySpawnVFX();
};
