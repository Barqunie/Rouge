// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "RougeAbilitySystemLibrary.generated.h"

class UProjectileInfo;
class UCharacterClassInfo;
/**
 * 
 */
UCLASS()
class ROUGE_API URougeAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintPure)
	static UCharacterClassInfo* GetCharacterClassDefaultInfo(const UObject* WorldContextObject) ;

	UFUNCTION(BlueprintPure)	
	static UProjectileInfo* GetProjectileInfo(const UObject* WorldContextObject);
};
