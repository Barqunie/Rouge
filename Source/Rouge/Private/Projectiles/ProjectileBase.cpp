// Fill out your copyright notice in the Description page of Project Settings.
#include "Projectiles/ProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include <Libraries/RougeAbilitySystemLibrary.h>
#include "Character/EnemyBase.h"
#include "Character/RougeCharacter.h"
#include "Net/UnrealNetwork.h"



AProjectileBase::AProjectileBase()
{

	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	// Misses don't live forever.
	InitialLifeSpan = 5.f;

	// Nothing changes after the spawn (clients simulate the flight themselves), the initial bunch is what matters.
	SetNetUpdateFrequency(10.f);

	// Visual only: hits come from the overlap sphere. No collision = no physics body per projectile, and the mesh
	// comes from the replicated Params, so the component itself doesn't need to replicate.
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	SetRootComponent(ProjectileMesh);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetCastShadow(false);


	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OvlerapSphere"));
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverlapSphere->SetupAttachment(RootComponent);
}


void AProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AProjectileBase, Params, COND_InitialOnly);
}

void AProjectileBase::SetProjectileParams(const FProjectileParams& InParams)
{
	Params = InParams;
	ApplyParams();
}

void AProjectileBase::ApplyParams()
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

void AProjectileBase::OnRep_Params()
{
	// Clients spawned it with the class defaults (speed already applied from those): restart along the facing with
	// the real speed. Arrives with the spawn (initial only), before BeginPlay.
	ApplyParams();
	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Velocity = GetActorForwardVector() * Params.InitialSpeed;
		ProjectileMovementComponent->UpdateComponentVelocity();
	}
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereBeginOverlap);

		// Spawned inside a target (point blank, e.g. an enemy hugging a turret): that overlap started before the
		// binding above and would never fire, so the shot would fly through.
		TArray<AActor*> Overlapping;
		OverlapSphere->GetOverlappingActors(Overlapping);
		for (AActor* Other : Overlapping)
		{
			if (TryHit(Other))
			{
				break;
			}
		}
	}
	else
	{
		// Only the server deals the hits; clients just draw the flight.
		OverlapSphere->SetGenerateOverlapEvents(false);
	}
}

void AProjectileBase::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryHit(OtherActor);
}

bool AProjectileBase::TryHit(AActor* OtherActor)
{
	if (bHasHit || !IsValid(OtherActor) || OtherActor == GetOwner()) return false;

	// Enemy projectiles hit players, player projectiles hit enemies. Dead characters are ignored.
	const bool bFiredByEnemy = GetOwner() && GetOwner()->IsA<AEnemyBase>();
	const bool bIsHostileTarget = bFiredByEnemy
		? OtherActor->IsA<ARougeCharacter>()
		: OtherActor->ActorHasTag(FName("Enemy"));

	const ACharacterBase* TargetCharacter = Cast<ACharacterBase>(OtherActor);
	if (!bIsHostileTarget || (TargetCharacter && TargetCharacter->IsDead())) return false;

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
	if (!TargetASC)
	{
		return false;
	}

	bHasHit = true;
	DamageEffectInfo.TargetASC = TargetASC;
	URougeAbilitySystemLibrary::ApplyDamageEffect(DamageEffectInfo);

	// Sonrasi icin duzenlenecek (iclerinden gecme vs.)
	Destroy();
	return true;
}
