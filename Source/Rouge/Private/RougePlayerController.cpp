// Copyright Epic Games, Inc. All Rights Reserved.


#include "RougePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Rouge.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include <Input/RougeInputComponent.h>
#include <Game/PlayerState/RougePlayerState.h>

#include <AbilitySystem/RougeAbilitySystemComponent.h>


void ARougePlayerController::BeginPlay()
{
	Super::BeginPlay();

	bReplicates = true;


	
}

void ARougePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}

		if(URougeInputComponent * RougeInputComp = Cast<URougeInputComponent>(InputComponent))
		{
			RougeInputComp->BindAbilityActions(RougeInputConfig, this, &ThisClass::AbilityInputPressed, &ThisClass::AbilityInputReleased);
		}
	}
}

UAbilitySystemComponent* ARougePlayerController::GetAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

URougeAbilitySystemComponent* ARougePlayerController::GetRougeAbilitySystemComponent()
{
	if(!IsValid(RougeAbilitySystemComp))
	{
		if (const ARougePlayerState* RougePlayerState = GetPlayerState<ARougePlayerState>())
		{
			RougeAbilitySystemComp = RougePlayerState->GetRougeAbilitySystemComponent();
		}
		
	}
	return RougeAbilitySystemComp;
}

bool ARougePlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ARougePlayerController::AbilityInputPressed(FGameplayTag InputTag)
{
	if(IsValid(GetRougeAbilitySystemComponent()))
	{
		RougeAbilitySystemComp->AbilityInputPressed(InputTag);
	}

}


void ARougePlayerController::AbilityInputReleased(FGameplayTag InputTag)
{
	if(IsValid(GetRougeAbilitySystemComponent()))
	{
		RougeAbilitySystemComp->AbilityInputReleased(InputTag);
	}
}
