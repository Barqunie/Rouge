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


UENUM(BlueprintType)
enum class ERougeTracerStyle : uint8
{
	/** Stretched bullet streaks (minigun spray). */
	Streak,
	/** A glowing orb with a trail of smaller ones (thrown chem orb). */
	Orb
};

/**
 * Drawn look of a projectile instead of its mesh: glowing streaks or an orb, plus muzzle flash and impact spark.
 * Turns purple while the shooter carries the package.
 */
USTRUCT(BlueprintType)
struct FRougeTracerLook
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer")
	bool bEnabled = false;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer")
	ERougeTracerStyle Style = ERougeTracerStyle::Streak;

	/** HDR: values above 1 bloom. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer")
	FLinearColor Color = FLinearColor(0.55f, 1.f, 0.1f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer", meta = (ClampMin = "0.0"))
	float Intensity = 8.f;

	/** Streak only. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer", meta = (ClampMin = "5.0"))
	float Length = 160.f;

	/** Streak thickness, or the orb's diameter. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer", meta = (ClampMin = "0.5"))
	float Width = 13.f;

	/** Cosmetic streaks (or shrinking orbs) trailing the real one, slightly off the line (no damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer", meta = (ClampMin = "0", ClampMax = "6"))
	int32 ExtraStreaks = 2;

	/** Distance between the streaks along the flight. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer", meta = (ClampMin = "0.0"))
	float StreakSpacing = 110.f;

	/** How far the extra streaks stray sideways. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer", meta = (ClampMin = "0.0"))
	float StreakSpread = 22.f;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer")
	bool bMuzzleFlash = true;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Tracer")
	bool bImpactSpark = true;
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

	/** When enabled the projectile draws as a bullet streak instead of ProjectileMesh. */
	UPROPERTY(EditDefaultsOnly)
	FRougeTracerLook Tracer;

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