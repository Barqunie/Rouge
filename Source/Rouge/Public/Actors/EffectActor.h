// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EffectActor.generated.h"

class UGameplayEffect;
class UBoxComponent;

UCLASS()
class ROUGE_API AEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	

	AEffectActor();

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:	

	UPROPERTY(VisibleAnywhere,meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> ActorMesh;

	UPROPERTY(VisibleAnywhere,meta = (AllowPrivateAccess = true))
	TObjectPtr<UBoxComponent> OverlapBox;

	UPROPERTY(EditAnywhere, Category = "Custom Value | Effects")
	TSubclassOf<UGameplayEffect> OverlapEffect;

};
