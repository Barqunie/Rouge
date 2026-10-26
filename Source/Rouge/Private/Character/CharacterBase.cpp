// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CharacterBase.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/RougeRetargetAnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Retargeter/IKRetargeter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UMaterialInterface* HitFlashMaterial = nullptr;
	const FName HitFlashColorParam(TEXT("Color"));
	const FName HitFlashOpacityParam(TEXT("Opacity"));
	const FName HitFlashIntensityParam(TEXT("Intensity"));
	const FName HitFlashRimParam(TEXT("RimAmount"));
}


ACharacterBase::ACharacterBase()
{

	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Same additive glow as the chempunk effects, used as an overlay on the mesh.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlashMaterial(TEXT("/Game/ThirdPerson/Materials/FX/M_ChemGlow.M_ChemGlow"));
	if (FlashMaterial.Succeeded())
	{
		HitFlashMaterial = FlashMaterial.Object;
	}

	// Empty unless RetargetedMesh is set (see PostInitializeComponents). Follows Mesh, so squash and hit stop carry over.
	VisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(GetMesh());
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	// Every enemy has one too (empty): no ticking until a look is set.
	VisualMesh->PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ACharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	USkeletalMeshComponent* SourceMesh = GetMesh();
	if (!VisualMesh || !SourceMesh || !RetargetedMesh || !RetargetAsset || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	VisualMesh->SetSkeletalMeshAsset(RetargetedMesh);
	VisualMesh->SetAnimInstanceClass(URougeRetargetAnimInstance::StaticClass());
	if (URougeRetargetAnimInstance* RetargetInstance = Cast<URougeRetargetAnimInstance>(VisualMesh->GetAnimInstance()))
	{
		RetargetInstance->Configure(RetargetAsset);
	}
	VisualMesh->AddTickPrerequisiteComponent(SourceMesh);
	VisualMesh->SetComponentTickEnabled(true);

	// The real mesh is the retarget source: it has to keep animating (and refreshing its bones) while not drawn.
	SourceMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	SourceMesh->SetHiddenInGame(true, false);
}

USkeletalMeshComponent* ACharacterBase::GetVisibleMesh() const
{
	return VisualMesh && VisualMesh->GetSkeletalMeshAsset() ? VisualMesh.Get() : GetMesh();
}


void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}
void ACharacterBase::InitAbilityActorInfo()
{

	
}

void ACharacterBase::BindCallbacksToDependencies()
{
}

void ACharacterBase::InitClassDefaults()
{
}

void ACharacterBase::BroadcastInitialValues()
{
}

void ACharacterBase::PlayHitReaction(bool bStrong)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || IsDead() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	HitStrength = bStrong ? 1.6f : 1.f;

	// Flash.
	if (HitFlashTime > 0.f && HitFlashMaterial)
	{
		if (!HitFlashMID)
		{
			HitFlashMID = UMaterialInstanceDynamic::Create(HitFlashMaterial, this);
			HitFlashMID->SetScalarParameterValue(HitFlashRimParam, 1.f);
		}
		HitFlashMID->SetVectorParameterValue(HitFlashColorParam, bStrong ? StrongHitFlashColor : HitFlashColor);
		HitFlashMID->SetScalarParameterValue(HitFlashIntensityParam, bStrong ? 3.f : 2.f);
		HitFlashMID->SetScalarParameterValue(HitFlashOpacityParam, 1.f);
		GetVisibleMesh()->SetOverlayMaterial(HitFlashMID);
		HitFlashRemaining = HitFlashTime;
	}

	// Squash.
	if (HitSquash > 0.f)
	{
		if (BaseMeshScale.IsZero())
		{
			BaseMeshScale = MeshComp->GetRelativeScale3D();
		}
		HitSquashElapsed = 0.f;
	}

	// Hit react animation (cosmetic, can cut an attack animation; gameplay runs on the server).
	const TArray<TObjectPtr<UAnimSequenceBase>>& Anims = (bStrong && StrongHitReactAnims.Num() > 0) ? StrongHitReactAnims : HitReactAnims;
	if (Anims.Num() > 0)
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			if (UAnimSequenceBase* Anim = Anims[FMath::RandRange(0, Anims.Num() - 1)])
			{
				AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, TEXT("DefaultSlot"), 0.05f, 0.2f, HitReactPlayRate);
			}
		}
	}

	// Hit stop: freeze the pose for a moment.
	if (HitPauseTime > 0.f)
	{
		HitPauseRemaining = HitPauseTime * (bStrong ? 1.8f : 1.f);
		MeshComp->GlobalAnimRateScale = 0.f;
	}
}

void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateHitReaction(DeltaTime);
}

void ACharacterBase::UpdateHitReaction(float DeltaTime)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	if (HitFlashRemaining > 0.f)
	{
		HitFlashRemaining -= DeltaTime;
		if (HitFlashRemaining <= 0.f)
		{
			GetVisibleMesh()->SetOverlayMaterial(nullptr);
		}
		else if (HitFlashMID)
		{
			HitFlashMID->SetScalarParameterValue(HitFlashOpacityParam, HitFlashRemaining / FMath::Max(HitFlashTime, 0.01f));
		}
	}

	if (HitSquashElapsed >= 0.f)
	{
		HitSquashElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(HitSquashElapsed / HitSquashTime, 0.f, 1.f);
		// Quick pop that springs back: wide and flat, then a little tall, then rest.
		const float Wave = FMath::Sin(Alpha * PI * 1.5f) * (1.f - Alpha) * HitSquash * HitStrength;
		MeshComp->SetRelativeScale3D(BaseMeshScale * FVector(1.f + Wave, 1.f + Wave, 1.f - Wave));
		if (Alpha >= 1.f)
		{
			MeshComp->SetRelativeScale3D(BaseMeshScale);
			HitSquashElapsed = -1.f;
		}
	}

	if (HitPauseRemaining > 0.f)
	{
		HitPauseRemaining -= DeltaTime;
		if (HitPauseRemaining <= 0.f)
		{
			MeshComp->GlobalAnimRateScale = 1.f;
		}
	}
}	


