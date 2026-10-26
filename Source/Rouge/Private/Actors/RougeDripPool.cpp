#include "Actors/RougeDripPool.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 MaxDrips = 320;
	constexpr float DripLifetime = 3.5f;
	constexpr float DripMeshSize = 100.f;
	const FLinearColor DripColor(0.06f, 1.f, 0.02f, 1.f);
	TWeakObjectPtr<ARougeDripPool> CachedDripPool;
}

ARougeDripPool::ARougeDripPool()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	Drops = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Drops"));
	RootComponent = Drops;
	Drops->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Drops->SetGenerateOverlapEvents(false);
	Drops->SetCastShadow(false);
	Drops->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/ThirdPerson/Materials/FX/M_ChemGlow.M_ChemGlow"));
	if (Cylinder.Succeeded())
	{
		Drops->SetStaticMesh(Cylinder.Object);
	}
	if (Glow.Succeeded())
	{
		Drops->SetMaterial(0, Glow.Object);
	}
}

void ARougeDripPool::AddDrop(UWorld* World, const FVector& FloorLocation, float Size)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	ARougeDripPool* PoolActor = CachedDripPool.Get();
	if (!PoolActor || PoolActor->GetWorld() != World)
	{
		PoolActor = nullptr;
		for (TActorIterator<ARougeDripPool> It(World); It; ++It)
		{
			PoolActor = *It;
			break;
		}
		if (!PoolActor)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;
			PoolActor = World->SpawnActor<ARougeDripPool>(ARougeDripPool::StaticClass(), FTransform::Identity, SpawnParams);
		}
		CachedDripPool = PoolActor;
	}
	if (PoolActor)
	{
		PoolActor->Add(FloorLocation, Size);
	}
}

void ARougeDripPool::Init()
{
	DropMID = Drops->CreateDynamicMaterialInstance(0);
	if (DropMID)
	{
		DropMID->SetVectorParameterValue(TEXT("Color"), DripColor);
		DropMID->SetScalarParameterValue(TEXT("Intensity"), 1.4f);
		DropMID->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		DropMID->SetScalarParameterValue(TEXT("RimAmount"), 0.f);
	}

	// Every slot exists from the start (scale 0); drops just reuse them.
	Pool.SetNum(MaxDrips);
	TArray<FTransform> Hidden;
	Hidden.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), MaxDrips);
	Drops->AddInstances(Hidden, false, true);
}

void ARougeDripPool::Add(const FVector& FloorLocation, float Size)
{
	if (Pool.Num() == 0)
	{
		Init();
	}
	FDrop& Drop = Pool[NextDrop];
	if (!Drop.bAlive)
	{
		++AliveDrops;
	}
	Drop.Location = FloorLocation + FVector(0.f, 0.f, 1.5f);
	Drop.Size = Size;
	Drop.Age = 0.f;
	Drop.bAlive = true;
	NextDrop = (NextDrop + 1) % Pool.Num();
}

void ARougeDripPool::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (AliveDrops == 0)
	{
		return;
	}

	// Splat in quickly, then shrink away.
	for (int32 Index = 0; Index < Pool.Num(); ++Index)
	{
		FDrop& Drop = Pool[Index];
		if (!Drop.bAlive)
		{
			continue;
		}
		Drop.Age += DeltaTime;
		float Scale = 0.f;
		if (Drop.Age >= DripLifetime)
		{
			Drop.bAlive = false;
			--AliveDrops;
		}
		else
		{
			const float In = FMath::Clamp(Drop.Age / 0.12f, 0.f, 1.f);
			const float Out = 1.f - FMath::Square(Drop.Age / DripLifetime);
			Scale = Drop.Size / DripMeshSize * In * Out;
		}
		Drops->UpdateInstanceTransform(Index, FTransform(FQuat::Identity, Drop.Location, FVector(Scale, Scale, 0.004f)), true, false, true);
	}
	Drops->MarkRenderStateDirty();
}
