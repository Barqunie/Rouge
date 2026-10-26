// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "Components/SphereComponent.h"
#include "ProjectileBase.generated.h"

struct FProjectileParams;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class ROUGE_API AProjectileBase : public AActor
{
	GENERATED_BODY()



public:

	AProjectileBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server, before FinishSpawning. Replicated with the spawn so clients fly with the same mesh / speed / gravity. */
	void SetProjectileParams(const FProjectileParams& InParams);

	UPROPERTY(BlueprintReadWrite)
	FDamageEffectInfo DamageEffectInfo;

private:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY()
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(ReplicatedUsing = OnRep_Params)
	FProjectileParams Params;

	/** Server: already hit something (several overlaps can come in the same frame). */
	bool bHasHit = false;

	UFUNCTION()
	void OnRep_Params();

	void ApplyParams();

	/** Server: damages OtherActor if it is a valid hostile target, then the projectile is gone. */
	bool TryHit(AActor* OtherActor);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);


};