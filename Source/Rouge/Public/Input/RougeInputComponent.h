// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/RougeInputConfig.h"
#include "RougeInputComponent.generated.h"

/**
 * 
 */
UCLASS()
class ROUGE_API URougeInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	template<class UserClass, typename PressedFuncType, typename ReleaseFuncType>
	void BindAbilityActions(URougeInputConfig* InputConfig, UserClass Object, PressedFuncType PressedFunc, ReleaseFuncType ReleaseFunc);

};

template<class UserClass, typename PressedFuncType, typename ReleaseFuncType>
void URougeInputComponent::BindAbilityActions(URougeInputConfig* InputConfig, UserClass Object, PressedFuncType PressedFunc, ReleaseFuncType ReleaseFunc)
{
	check(InputConfig);

	for (const FRougeInputAction Action : InputConfig->RougeInputActions)
	{
		if (IsValid(Action.InputAction) && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Started, Object, PressedFunc, Action.InputTag);
			}
			if (ReleaseFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleaseFunc, Action.InputTag);
			}
		}
	}
}