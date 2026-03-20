// Fill out your copyright notice in the Description page of Project Settings.
#include "Projectiles/ProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include <Libraries/RougeAbilitySystemLibrary.h>



AProjectileBase::AProjectileBase()
{

	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	SetRootComponent(ProjectileMesh);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProjectileMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	//ProjectileMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ProjectileMesh->SetIsReplicated(true);


	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OvlerapSphere"));
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverlapSphere->SetupAttachment(RootComponent);
}


void AProjectileBase::SetProjectileParams(const FProjectileParams& Params)
{
	if (IsValid(ProjectileMesh))
	{
		ProjectileMesh->SetStaticMesh(Params.ProjectileMesh);
	}
	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->InitialSpeed = Params.InitialSpeed;
		ProjectileMovementComponent->MaxSpeed = Params.InitialSpeed;
		ProjectileMovementComponent->ProjectileGravityScale = Params.GravityScale;
		ProjectileMovementComponent->bShouldBounce = Params.bShouldBounce;
		ProjectileMovementComponent->Bounciness = Params.Bounciness;
	}
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereBeginOverlap);
	}
}

void AProjectileBase::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == GetOwner()) return;

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor))
	{
		if (OtherActor->ActorHasTag(FName("Enemy"))) {
			DamageEffectInfo.TargetASC = TargetASC;
			URougeAbilitySystemLibrary::ApplyDamageEffect(DamageEffectInfo);

			//Sonrasi için düzenlenecek(içlerinden geçme vs.)
			Destroy();
		}

	}
}