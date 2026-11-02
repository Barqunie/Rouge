// The package the team escorts to the delivery zone.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AttributeSet.h"
#include "RougePackage.generated.h"

class ACharacter;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPackageCarrierChanged, ACharacter*, NewCarrier);

/**
 * Server authoritative. On the ground, the closest player (or an enemy, when bEnemiesCanCarry) walking into
 * PickupRadius picks it up; it then rides above the carrier's head (attachment replicates).
 * Carriers drop it on death; players can drop it on purpose (F). The one who dropped it can't grab it back
 * for RepickDelay seconds.
 */
UCLASS()
class ROUGE_API ARougePackage : public AActor
{
	GENERATED_BODY()

public:
	ARougePackage();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	/** The package in this world (there is one per run), or null. */
	static ARougePackage* Find(const UWorld* World);

	UFUNCTION(BlueprintPure, Category = "Rouge|Package")
	ACharacter* GetCarrier() const { return Carrier; }

	UFUNCTION(BlueprintPure, Category = "Rouge|Package")
	bool IsOnGround() const { return Carrier == nullptr; }

	UFUNCTION(BlueprintPure, Category = "Rouge|Package")
	bool IsCarriedByPlayer() const;

	UFUNCTION(BlueprintPure, Category = "Rouge|Package")
	bool IsCarriedByEnemy() const;

	bool CanEnemiesCarry() const { return bEnemiesCanCarry; }

	/** Server only. Returns false if it is already carried or this character can't take it right now. */
	bool TryPickUp(ACharacter* NewCarrier);

	/** Server only. Puts the package on the ground under its carrier. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Rouge|Package")
	void Drop();

	/**
	 * Server only. Sets the package on a station's pedestal. While docked players can't pick it up (it is being
	 * purified); enemies still can steal it, which undocks it.
	 */
	void DockAt(AActor* Station, const FVector& DockLocation);

	/** Server only. Released after purification: players can take it again. */
	void Undock();

	UFUNCTION(BlueprintPure, Category = "Rouge|Package")
	AActor* GetDockedStation() const { return DockedStation; }

	/** Fires on every machine when the carrier changes (null = on the ground). */
	UPROPERTY(BlueprintAssignable, Category = "Rouge|Package")
	FOnPackageCarrierChanged OnCarrierChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Rouge|Package")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	/** Glowing circle under the package while it lies on the ground. */
	UPROPERTY(VisibleAnywhere, Category = "Rouge|Package")
	TObjectPtr<UStaticMeshComponent> GroundRing;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package", meta = (ClampMin = "10.0"))
	float PickupRadius = 110.f;

	/** Player carriers move at this fraction of their speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CarrierSpeedMultiplier = 0.85f;

	// ---------------- Carry buff (by class, while a player holds it) ----------------

	/** Fighter: + life steal (percent of damage dealt). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package|Carry Buff", meta = (ClampMin = "0.0"))
	float CarryLifeSteal = 12.f;

	/** Tank: + max health as a fraction of it (also heals that much on pickup). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package|Carry Buff", meta = (ClampMin = "0.0"))
	float CarryMaxHealthFraction = 0.3f;

	/** Ranged: + attack speed (0.25 = 25% faster). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package|Carry Buff", meta = (ClampMin = "0.0"))
	float CarryAttackSpeed = 0.25f;

	/** Support: moves at this speed while carrying instead of being slowed. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package|Carry Buff", meta = (ClampMin = "0.1"))
	float SupportCarrySpeedMultiplier = 1.15f;

public:
	/** Short text of the carry buff the character would get (HUD), empty for non players. */
	FString DescribeCarryBuff(const ACharacter* Character) const;

protected:

	/** Enemies walking into it steal it. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package")
	bool bEnemiesCanCarry = true;

	/** Seconds before whoever dropped it can pick it up again. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package", meta = (ClampMin = "0.0"))
	float RepickDelay = 1.f;

	/** On the carrier's back: offset from the carrier's center (X forward, Z up). Used without a socket. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package")
	FVector CarryOffset = FVector(-38.f, 0.f, 20.f);

	/**
	 * Optional bone / socket on the carrier's mesh (e.g. a "PackageSocket" added to the skeleton) so the package
	 * follows the animation. Used when the carrier's mesh has it; offset/rotation are then relative to the socket.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package")
	FName CarrySocketName = TEXT("PackageSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package")
	FVector CarrySocketOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package")
	FRotator CarrySocketRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Package")
	FLinearColor PackageColor = FLinearColor(1.f, 0.7f, 0.15f, 1.f);

private:
	struct FCarryBuffMod
	{
		FGameplayAttribute Attribute;
		float Amount = 0.f;
	};

	/** Server: attribute changes given to the current carrier, taken back when it changes. */
	TArray<FCarryBuffMod> AppliedCarryMods;
	TWeakObjectPtr<class UAbilitySystemComponent> CarryBuffASC;

	void ApplyCarryBuff(ACharacter* NewCarrier);
	void RemoveCarryBuff();

	UPROPERTY(ReplicatedUsing = OnRep_Carrier)
	TObjectPtr<ACharacter> Carrier;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Rouge|Package")
	TObjectPtr<AActor> DockedStation;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BoxMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	TWeakObjectPtr<ACharacter> LastDropper;
	float RepickAllowedTime = 0.f;
	FVector LastCarrierLocation = FVector::ZeroVector;
	float NextPickupCheckTime = 0.f;
	float Age = 0.f;

	UFUNCTION()
	void OnRep_Carrier();

	void SetCarrier(ACharacter* NewCarrier);
	bool CanBeCarriedBy(const ACharacter* Character) const;
	void CheckForPickup();
	void PlaceOnGround(const FVector& Near);
	void UpdateVisuals();
};
