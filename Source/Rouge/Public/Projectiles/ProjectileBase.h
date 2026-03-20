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



	void SetProjectileParams(const FProjectileParams& Params);

	UPROPERTY(BlueprintReadWrite)
	FDamageEffectInfo DamageEffectInfo;

private:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY()
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);


};