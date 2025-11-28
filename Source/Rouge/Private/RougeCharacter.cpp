// Copyright Epic Games, Inc. All Rights Reserved.

#include "RougeCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Rouge.h"
#include "Data/CharacterClassInfo.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include <Game/PlayerState/RougePlayerState.h>
#include "AbilitySystem/RougeAbilitySystemComponent.h"

#include "AbilitySystem/Attributes/RougeAttributeSet.h"



ARougeCharacter::ARougeCharacter()
{


	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;


	GetCharacterMovement()->bOrientRotationToMovement =  false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);


	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;


	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 800.0f;
	CameraBoom->SetRelativeRotation(FRotator(-45.f,0.f , 0.f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->SetUsingAbsoluteRotation(true);   
	CameraBoom->bDoCollisionTest = false;



	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;


}

void ARougeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARougeCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::None, this, &ARougeCharacter::Look);

	}
	else
	{
		UE_LOG(LogRouge, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ARougeCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* RougePlayerController = Cast<APlayerController>(GetController()))
	{
		RougePlayerController->bShowMouseCursor = true;

		FInputModeGameOnly InputMode;
		RougePlayerController->SetInputMode(InputMode);
	}
}


void ARougeCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void ARougeCharacter::Look(const FInputActionValue& Value)
{
	APlayerController* RougePlayerController = Cast<APlayerController>(GetController());
	if (!RougePlayerController) return;

	FHitResult Hit;

	if (RougePlayerController->GetHitResultUnderCursor(ECC_Visibility, false ,Hit))
	{
		const FVector ActorLocation = GetActorLocation();

		FVector TargetLocation = Hit.ImpactPoint;
		TargetLocation.Z = ActorLocation.Z;    

		FVector ToTarget = TargetLocation - ActorLocation;
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator NewRotation = ToTarget.Rotation();
			SetActorRotation(NewRotation);
		}
	}
}

void ARougeCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		const FVector ForwardDirection = FVector::ForwardVector;
		const FVector RightDirection = FVector::RightVector;

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ARougeCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{

		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}




void ARougeCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if(HasAuthority())
	{
		InitAbilityActorInfo();
	}
}

void ARougeCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
}

UAbilitySystemComponent* ARougeCharacter::GetAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

void ARougeCharacter::InitAbilityActorInfo()
{
	if(ARougePlayerState* RougePlayerState = GetPlayerState<ARougePlayerState>())
	{
		RougeAbilitySystemComp = RougePlayerState->GetRougeAbilitySystemComponent();
		RougeAttributes = RougePlayerState->GetRougeAttributeSet();


		if(IsValid(RougeAbilitySystemComp))
		{
			RougeAbilitySystemComp->InitAbilityActorInfo(RougePlayerState, this);
			BindCallbacksToDependencies();  BroadcastInitialValues();

			if(HasAuthority())
			{
				InitClassDefaults();
			}
		}
	}
}

void ARougeCharacter::InitClassDefaults()
{
	if(!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("'%s' CharacterTag is not valid! Please set a valid GameplayTag for the character in the class defaults."), *GetNameSafe(this));
	}

	else if (UCharacterClassInfo* ClassInfo = URougeAbilitySystemLibrary::GetCharacterClassDefaultInfo(this))
	{
		if (const FCharacterClassDefaultInfo* SelectedClassInfo = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
		{
			if(IsValid(RougeAbilitySystemComp))
			{
				RougeAbilitySystemComp->AddCharacterAbilities(SelectedClassInfo->StartingAbilities);
				RougeAbilitySystemComp->AddCharacterPassiveAbilities(SelectedClassInfo->PassiveAbilities);
				RougeAbilitySystemComp->InitializeDefaultAttributes(SelectedClassInfo->DefaultAttributes);
			}
		}
	}
}

void ARougeCharacter::BindCallbacksToDependencies()
{
	if (IsValid(RougeAbilitySystemComp) && IsValid(RougeAttributes))
	{
		RougeAbilitySystemComp->GetGameplayAttributeValueChangeDelegate(RougeAttributes->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.NewValue, RougeAttributes->GetMaxHealth());
			});

		  RougeAbilitySystemComp->GetGameplayAttributeValueChangeDelegate(RougeAttributes->GetXPAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnXPChanged(Data.NewValue,RougeAttributes->GetMaxXP());
			});
	}
}

void ARougeCharacter::BroadcastInitialValues()
{
	if (IsValid(RougeAttributes))
	{
		OnHealthChanged(RougeAttributes->GetHealth(), RougeAttributes->GetMaxHealth());
		OnXPChanged(RougeAttributes->GetXP(), RougeAttributes->GetMaxXP());
	}
}

