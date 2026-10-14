// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeAreaIndicator.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Short-lived flat circle on the ground showing where an area skill hits.
 * Spawned on the server with SpawnIndicator; replicates to clients and fades out locally.
 */
UCLASS()
class ROUGE_API ARougeAreaIndicator : public AActor
{
	GENERATED_BODY()

public:
	ARougeAreaIndicator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	/** Server only. Places the circle on the ground under Location. */
	static ARougeAreaIndicator* SpawnIndicator(UWorld* World, const FVector& Location, float InRadius, const FLinearColor& InColor, float InDuration);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Indicator")
	TObjectPtr<UStaticMeshComponent> CircleMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	float Radius = 200.f;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	FLinearColor Color = FLinearColor(1.f, 0.5f, 0.1f, 0.5f);

	UPROPERTY(Replicated)
	float Duration = 0.5f;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CircleMID;

	float Age = 0.f;

	UFUNCTION()
	void OnRep_Visuals();

	void ApplyVisuals();
};
