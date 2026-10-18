// Boomerang thrown by the Boomerang passive.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "RougeBoomerang.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Flies straight out, then back to its owner, damaging each enemy once per leg (server).
 * Movement runs on the server and replicates; clients only spin the mesh.
 */
UCLASS()
class ROUGE_API ARougeBoomerang : public AActor
{
	GENERATED_BODY()

public:
	ARougeBoomerang();

	/** Server, before FinishSpawning. */
	void Init(const FDamageEffectInfo& InDamageInfo, const FVector& InDirection, float InTravelDistance, float InSpeed);

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Boomerang")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Boomerang")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boomerang")
	FLinearColor Color = FLinearColor(1.f, 0.55f, 0.1f, 1.f);

	/** Visual spin, degrees per second. */
	UPROPERTY(EditDefaultsOnly, Category = "Boomerang")
	float SpinSpeed = 900.f;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MeshMID;

	FDamageEffectInfo DamageInfo;
	FVector Direction = FVector::ForwardVector;
	float TravelDistance = 600.f;
	float Speed = 1000.f;
	float Traveled = 0.f;
	bool bReturning = false;

	/** Enemies already hit on the current leg. */
	TSet<TWeakObjectPtr<AActor>> HitThisLeg;

	UFUNCTION()
	void OnCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void TryHit(AActor* OtherActor);
};
