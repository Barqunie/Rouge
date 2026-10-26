// Chem turret placed by the ranged character's E: shoots the nearest enemy for a while.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "RougeTurret.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Server-side setup from the skill. */
USTRUCT()
struct FRougeTurretSettings
{
	GENERATED_BODY()

	float Duration = 8.f;
	float FireInterval = 0.6f;
	float Range = 950.f;

	/** Projectiles per shot (fanned out a little). */
	int32 ShotsPerVolley = 1;

	UPROPERTY()
	FProjectileParams Projectile;

	/** Per projectile damage (already includes the skill's upgrades). */
	UPROPERTY()
	FDamageEffectInfo DamageInfo;
};

/**
 * Spawned on the server by URougeTurretSkillAbility. Picks the nearest alive enemy in Range, turns the head
 * toward it and fires the skill's projectile every FireInterval. Replicates its target so clients aim the head,
 * plus a cosmetic multicast per shot (recoil + muzzle glow). Destroys itself after Duration.
 */
UCLASS()
class ROUGE_API ARougeTurret : public AActor
{
	GENERATED_BODY()

public:
	ARougeTurret();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	/** Server only. Placed on the ground under Location. */
	static ARougeTurret* SpawnTurret(UWorld* World, AActor* InOwner, const FVector& Location, const FRotator& Facing,
		const FRougeTurretSettings& InSettings);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Rotates toward the target; head, barrel and vial hang off it (unscaled, so they keep their own sizes). */
	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<USceneComponent> HeadPivot;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> BarrelMesh;

	/** Glowing chem vial on top. */
	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> VialMesh;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> RangeRing;

	/** Seconds left (replicated so clients can blink the vial at the end). */
	UPROPERTY(Replicated)
	float Lifetime = 8.f;

	UPROPERTY(Replicated)
	float Range = 950.f;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> Target;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MetalMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> VialMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	UPROPERTY()
	FRougeTurretSettings Settings;

	FTimerHandle FireTimer;
	float Age = 0.f;
	float Recoil = 0.f;
	float HeadYaw = 0.f;

	/** Server: pick a target and shoot. */
	void Fire();
	AActor* FindTarget() const;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFired();
};
