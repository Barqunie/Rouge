#include "Projectiles/RougeBoomerang.h"

#include "AbilitySystemGlobals.h"
#include "Character/EnemyBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameplayEffect.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARougeBoomerang::ARougeBoomerang()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(60.f);
	InitialLifeSpan = 8.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(45.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	SetRootComponent(Collision);

	// Flat spinning disc (engine cylinder), tinted through BasicShapeMaterial's "Color" parameter.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetRelativeScale3D(FVector(0.7f, 0.25f, 0.06f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DiscMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DiscMesh.Succeeded())
	{
		Mesh->SetStaticMesh(DiscMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ShapeMaterial.Object);
	}
}

void ARougeBoomerang::Init(const FDamageEffectInfo& InDamageInfo, const FVector& InDirection, float InTravelDistance, float InSpeed)
{
	DamageInfo = InDamageInfo;
	Direction = InDirection.GetSafeNormal2D(UE_SMALL_NUMBER);
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::ForwardVector;
	}
	TravelDistance = InTravelDistance;
	Speed = InSpeed;
}

void ARougeBoomerang::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* Material = Mesh->GetMaterial(0))
	{
		MeshMID = Mesh->CreateDynamicMaterialInstance(0, Material);
		if (MeshMID)
		{
			MeshMID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}

	if (HasAuthority())
	{
		Collision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCollisionOverlap);
	}
}

void ARougeBoomerang::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Mesh->AddLocalRotation(FRotator(0.f, SpinSpeed * DeltaTime, 0.f));

	if (!HasAuthority())
	{
		return;
	}

	const float Step = Speed * DeltaTime;

	if (!bReturning)
	{
		SetActorLocation(GetActorLocation() + Direction * Step);
		Traveled += Step;
		if (Traveled >= TravelDistance)
		{
			bReturning = true;
			HitThisLeg.Reset();
		}
		return;
	}

	const AActor* Thrower = GetOwner();
	if (!IsValid(Thrower))
	{
		Destroy();
		return;
	}

	FVector ToThrower = Thrower->GetActorLocation() - GetActorLocation();
	ToThrower.Z = 0.f;
	if (ToThrower.Size() <= FMath::Max(Step, 60.f))
	{
		Destroy();
		return;
	}

	// Comes back a little faster than it went out.
	SetActorLocation(GetActorLocation() + ToThrower.GetSafeNormal() * Step * 1.2f);
}

void ARougeBoomerang::OnCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryHit(OtherActor);
}

void ARougeBoomerang::TryHit(AActor* OtherActor)
{
	AEnemyBase* Enemy = Cast<AEnemyBase>(OtherActor);
	if (!Enemy || Enemy->IsDead() || HitThisLeg.Contains(Enemy))
	{
		return;
	}
	HitThisLeg.Add(Enemy);

	FDamageEffectInfo Hit = DamageInfo;
	Hit.TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Enemy);
	if (Hit.TargetASC && Hit.DamageEffect && Hit.BaseDamage > 0.f)
	{
		URougeAbilitySystemLibrary::ApplyDamageEffect(Hit);
	}
}
