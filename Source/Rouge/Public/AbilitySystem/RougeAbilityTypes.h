// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "RougeAbilityTypes.generated.h"

class AProjectileBase;
class UGameplayEffect;
class UAbilitySystemComponent;
/**
 * 
 */

USTRUCT()
struct FRougeGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	bool IsCriticalHit() const { return bCriticalHit; }

	void SetIsCriticalHit(const bool InCriticalHit) { bCriticalHit = InCriticalHit; }

	static ROUGE_API
		FRougeGameplayEffectContext* GetEffectContext(FGameplayEffectContextHandle Handle);

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FRougeGameplayEffectContext* Duplicate() const override
	{
		FRougeGameplayEffectContext* NewContext = new FRougeGameplayEffectContext();
		*NewContext = *this;

		if(GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(),true);
		}
		return NewContext;
	}


	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

private :


	UPROPERTY()	
	bool bCriticalHit = false;
};



template<>
struct TStructOpsTypeTraits<FRougeGameplayEffectContext> : TStructOpsTypeTraitsBase2<FRougeGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true

	};
};


USTRUCT()
struct FProjectileParams
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly)
	float InitialSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly)
	float GravityScale = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bShouldBounce = false;

	UPROPERTY(EditDefaultsOnly)
	float Bounciness = 0.6f;

};

USTRUCT(BlueprintType)
struct FDamageEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> AvatarActor = nullptr;	

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> DamageEffect = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;	

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> TargetASC = nullptr;


	UPROPERTY(BlueprintReadWrite)
	float BaseDamage = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float AbilityLevel = 1.f;

};