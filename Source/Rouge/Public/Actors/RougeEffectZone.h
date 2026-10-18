// Lingering ground zone (support E): speeds up players inside and burns enemies inside.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "RougeEffectZone.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UNiagaraSystem;

/** Server-side settings of a zone (filled by the skill). */
USTRUCT()
struct FRougeZoneSettings
{
	GENERATED_BODY()

	float Radius = 400.f;
	float Duration = 5.f;
	float TickInterval = 0.5f;

	/** Move speed multiplier for players inside (1.3 = 30% faster). 1 = none. */
	float AllySpeedMultiplier = 1.f;

	/** Per tick damage to enemies inside (DamageInfo.BaseDamage is used, 0 = none). */
	UPROPERTY()
	FDamageEffectInfo DamageInfo;
};

/**
 * Spawned on the server by URougeZoneSkillAbility. Every TickInterval it refreshes the speed buff on players
 * inside and damages enemies inside. Clients get the radius / color / effect through replication and draw the
 * glowing disc and the fire puffs themselves.
 */
UCLASS()
class ROUGE_API ARougeEffectZone : public AActor
{
	GENERATED_BODY()

public:
	ARougeEffectZone();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	/** Server only. Places the zone on the ground under Location. */
	static ARougeEffectZone* SpawnZone(UWorld* World, const FVector& Location, const FRougeZoneSettings& InSettings,
		const FLinearColor& InColor, UNiagaraSystem* InVFX, float InVFXScale, float InVFXInterval);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UStaticMeshComponent> DiscMesh;

	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UStaticMeshComponent> EdgeMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	float Radius = 400.f;

	UPROPERTY(Replicated)
	float Duration = 5.f;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	FLinearColor Color = FLinearColor(1.f, 0.55f, 0.1f, 0.3f);

	/** Spawned at random points inside the zone (cosmetic). */
	UPROPERTY(Replicated)
	TObjectPtr<UNiagaraSystem> AmbientVFX;

	UPROPERTY(Replicated)
	float VFXScale = 0.35f;

	UPROPERTY(Replicated)
	float VFXInterval = 0.3f;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DiscMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> EdgeMID;

	UPROPERTY()
	FRougeZoneSettings Settings;

	FTimerHandle PulseTimer;
	float Age = 0.f;
	float VFXCountdown = 0.f;

	UFUNCTION()
	void OnRep_Visuals();

	void ApplyVisuals();

	/** Server: one tick of speed buff + burn. */
	void Pulse();
};
