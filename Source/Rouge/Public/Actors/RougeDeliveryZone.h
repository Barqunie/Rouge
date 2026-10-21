// Where the package has to go (city center). Place one in the level.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeDeliveryZone.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Server: when the package is inside Radius while a player carries it (or it lies there), the run is won
 * (ARougeMainGameMode::HandlePackageDelivered). Clients only draw the glowing circle and the light pillar.
 */
UCLASS()
class ROUGE_API ARougeDeliveryZone : public AActor
{
	GENERATED_BODY()

public:
	ARougeDeliveryZone();

	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	static ARougeDeliveryZone* Find(const UWorld* World);

	UFUNCTION(BlueprintPure, Category = "Delivery")
	float GetRadius() const { return Radius; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Delivery")
	TObjectPtr<UStaticMeshComponent> CircleMesh;

	UPROPERTY(VisibleAnywhere, Category = "Delivery")
	TObjectPtr<UStaticMeshComponent> PillarMesh;

	UPROPERTY(EditAnywhere, Category = "Delivery", meta = (ClampMin = "50.0"))
	float Radius = 350.f;

	UPROPERTY(EditAnywhere, Category = "Delivery")
	FLinearColor ZoneColor = FLinearColor(0.3f, 1.f, 0.45f, 0.3f);

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CircleMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PillarMID;

	float Age = 0.f;
	float NextCheckTime = 0.f;

	void ApplySize();
};
