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
#include "Actors/RougeChemFX.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine basic shapes are 100 units across.
	constexpr float TracerShapeSize = 100.f;

	UStaticMesh* TracerCylinderMesh = nullptr;
	UMaterialInterface* TracerGlowMaterial = nullptr;
}


AProjectileBase::AProjectileBase()
{

	// Only tracers tick (to grow their streaks out of the muzzle), switched on in BuildTracer.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/Materials/FX/M_ChemGlow.M_ChemGlow"));
	TracerCylinderMesh = Cylinder.Succeeded() ? Cylinder.Object : nullptr;
	TracerGlowMaterial = Glow.Succeeded() ? Glow.Object : nullptr;

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
		ProjectileMesh->SetStaticMesh(Params.Tracer.bEnabled ? nullptr : Params.ProjectileMesh.Get());
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

	if (Params.Tracer.bEnabled && GetNetMode() != NM_DedicatedServer)
	{
		BuildTracer();
	}
}

void AProjectileBase::BuildTracer()
{
	const FRougeTracerLook& Look = Params.Tracer;
	if (!TracerCylinderMesh || !TracerGlowMaterial)
	{
		return;
	}

	auto MakeGlow = [this](const FLinearColor& Color, float Intensity)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(TracerGlowMaterial, this);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Intensity"), Intensity);
		MID->SetScalarParameterValue(TEXT("RimAmount"), 0.f);
		MID->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		return MID;
	};
	UMaterialInstanceDynamic* GlowMID = MakeGlow(Look.Color, Look.Intensity);
	UMaterialInstanceDynamic* CoreMID = MakeGlow(FMath::Lerp(Look.Color, FLinearColor::White, 0.65f), Look.Intensity * 1.6f);

	// Offsets are in world units: undo any scale on the root.
	const FVector RootScale = GetActorScale3D().GetAbs().ComponentMax(FVector(KINDA_SMALL_NUMBER));

	// Lying cylinders (axis along the flight), centred behind the head: Offset = where the streak's front is.
	auto AddStreak = [&](UMaterialInstanceDynamic* MID, float Width, float Length, const FVector& Front)
	{
		UStaticMeshComponent* Streak = NewObject<UStaticMeshComponent>(this);
		Streak->SetStaticMesh(TracerCylinderMesh);
		Streak->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Streak->SetGenerateOverlapEvents(false);
		Streak->SetCastShadow(false);
		Streak->SetupAttachment(RootComponent);
		Streak->SetUsingAbsoluteScale(true);
		Streak->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		Streak->SetRelativeLocation((Front - FVector(Length * 0.5f, 0.f, 0.f)) / RootScale);
		Streak->SetRelativeScale3D(FVector(Width / TracerShapeSize, Width / TracerShapeSize, Length / TracerShapeSize));
		Streak->SetMaterial(0, MID);
		Streak->SetVisibility(false);
		Streak->RegisterComponent();
		TracerStreaks.Add(Streak);
		TracerStreakOffsets.Add(-Front.X + Length);
	};

	AddStreak(GlowMID, Look.Width, Look.Length, FVector::ZeroVector);
	AddStreak(CoreMID, Look.Width * 0.4f, Look.Length * 0.9f, FVector::ZeroVector);
	for (int32 Index = 1; Index <= Look.ExtraStreaks; ++Index)
	{
		const FVector Front(-Look.StreakSpacing * Index,
			FMath::FRandRange(-Look.StreakSpread, Look.StreakSpread),
			FMath::FRandRange(-Look.StreakSpread, Look.StreakSpread) * 0.4f);
		AddStreak(GlowMID, Look.Width * 0.6f, Look.Length * FMath::FRandRange(0.55f, 0.8f), Front);
	}

	TracerStart = GetActorLocation();
	SetActorTickEnabled(true);

	if (Look.bMuzzleFlash)
	{
		FRougeChemFXSettings Flash;
		Flash.Color = FMath::Lerp(Look.Color, FLinearColor::White, 0.4f);
		Flash.Intensity = Look.Intensity;
		Flash.Radius = Look.Width * 1.2f;
		Flash.Duration = 0.07f;
		Flash.BubbleCount = 0;
		ARougeChemFX::SpawnBurst(GetWorld(), TracerStart, Flash);
	}
}

void AProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// A streak shows once its whole length is out of the muzzle (no streaks poking out behind the shooter).
	const float Flown = FVector::Dist(GetActorLocation(), TracerStart);
	bool bAllShown = true;
	for (int32 Index = 0; Index < TracerStreaks.Num(); ++Index)
	{
		const bool bShow = Flown >= TracerStreakOffsets[Index];
		if (TracerStreaks[Index] && TracerStreaks[Index]->IsVisible() != bShow)
		{
			TracerStreaks[Index]->SetVisibility(bShow);
		}
		bAllShown &= bShow;
	}
	if (bAllShown)
	{
		SetActorTickEnabled(false);
	}
}

void AProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Spark where the shot ended (a hit or a wall), not when a miss simply runs out of life far away.
	const bool bExpired = InitialLifeSpan > 0.f && GetGameTimeSinceCreation() >= InitialLifeSpan - 0.1f;
	if (EndPlayReason == EEndPlayReason::Destroyed && !bExpired && Params.Tracer.bEnabled && Params.Tracer.bImpactSpark
		&& GetNetMode() != NM_DedicatedServer)
	{
		FRougeChemFXSettings Spark;
		Spark.Color = Params.Tracer.Color;
		Spark.Intensity = Params.Tracer.Intensity * 0.8f;
		Spark.Radius = Params.Tracer.Width * 1.6f;
		Spark.Duration = 0.18f;
		Spark.BubbleCount = 3;
		Spark.BubbleSize = Params.Tracer.Width;
		Spark.BubbleRise = 45.f;
		ARougeChemFX::SpawnBurst(GetWorld(), GetActorLocation(), Spark);
	}

	Super::EndPlay(EndPlayReason);
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
