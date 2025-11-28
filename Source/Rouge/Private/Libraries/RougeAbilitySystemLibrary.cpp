// Fill out your copyright notice in the Description page of Project Settings.

#include "Libraries/RougeAbilitySystemLibrary.h"
#include <Game/RougeMainGameMode.h>
#include <Kismet/GameplayStatics.h>


UCharacterClassInfo* URougeAbilitySystemLibrary::GetCharacterClassDefaultInfo(const UObject* WorldContextObject)
{
	if(const ARougeMainGameMode* RougeMainGameMode = Cast<ARougeMainGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return RougeMainGameMode->GetCharacterClassDefaultInfo();
	}

	return nullptr;
}
