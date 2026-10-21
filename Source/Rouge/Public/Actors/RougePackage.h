// The package the team escorts to the delivery zone.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	UFUNCTION(BlueprintPure, Category = "Package")
	ACharacter* GetCarrier() const { return Carrier; }

	UFUNCTION(BlueprintPure, Category = "Package")
	bool IsOnGround() const { return Carrier == nullptr; }

	UFUNCTION(BlueprintPure, Category = "Package")
	bool IsCarriedByPlayer() const;

	UFUNCTION(BlueprintPure, Category = "Package")
	bool IsCarriedByEnemy() const;

	bool CanEnemiesCarry() const { return bEnemiesCanCarry; }

	/** Server only. Returns false if it is already carried or this character can't take it right now. */
	bool TryPickUp(ACharacter* NewCarrier);

	/** Server only. Puts the package on the ground under its carrier. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Package")
	void Drop();

	/** Fires on every machine when the carrier changes (null = on the ground). */
	UPROPERTY(BlueprintAssignable, Category = "Package")
	FOnPackageCarrierChanged OnCarrierChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Package")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	/** Glowing circle under the package while it lies on the ground. */
	UPROPERTY(VisibleAnywhere, Category = "Package")
	TObjectPtr<UStaticMeshComponent> GroundRing;

	UPROPERTY(EditDefaultsOnly, Category = "Package", meta = (ClampMin = "10.0"))
	float PickupRadius = 110.f;

	/** Player carriers move at this fraction of their speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Package", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CarrierSpeedMultiplier = 0.85f;

	/** Enemies walking into it steal it. */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	bool bEnemiesCanCarry = true;

	/** Seconds before whoever dropped it can pick it up again. */
	UPROPERTY(EditDefaultsOnly, Category = "Package", meta = (ClampMin = "0.0"))
	float RepickDelay = 1.f;

	/** Where it sits relative to the carrier's center. */
	UPROPERTY(EditDefaultsOnly, Category = "Package")
	FVector CarryOffset = FVector(0.f, 0.f, 125.f);

	UPROPERTY(EditDefaultsOnly, Category = "Package")
	FLinearColor PackageColor = FLinearColor(1.f, 0.7f, 0.15f, 1.f);

private:
	UPROPERTY(ReplicatedUsing = OnRep_Carrier)
	TObjectPtr<ACharacter> Carrier;

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
