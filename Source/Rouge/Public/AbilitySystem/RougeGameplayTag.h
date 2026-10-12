// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"


namespace RougeGameplayTags::Combat
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
}

namespace RougeGameplayTags::Event
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee_WindowOpen);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee_WindowClose);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee_Hit);
}
