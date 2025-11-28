// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/EffectActor.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

// Sets default values
AEffectActor::AEffectActor()
{

	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	ActorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActorMesh"));
	SetRootComponent(ActorMesh);

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	OverlapBox->SetupAttachment(GetRootComponent());
	OverlapBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

}


void AEffectActor::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		if(OverlapBox)
		{
			OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &AEffectActor::OnBoxBeginOverlap);
		}
	}
	
}

void AEffectActor::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		FGameplayEffectContextHandle ContextHandle = OtherASC->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = OtherASC->MakeOutgoingSpec(OverlapEffect, 1.f,ContextHandle);
		OtherASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		Destroy();
	}
}



