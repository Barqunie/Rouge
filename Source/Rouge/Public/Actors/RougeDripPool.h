// Glowing toxic drops left on the floor (ghoul trails): one instanced mesh for every drop on this machine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeDripPool.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Local only (never replicated), spawned on first use. Drops live in a ring buffer of instances that shrink and
 * vanish over DropLifetime, so a whole swarm costs one draw call.
 */
UCLASS(NotBlueprintable, Transient)
class ROUGE_API ARougeDripPool : public AActor
{
	GENERATED_BODY()

public:
	ARougeDripPool();

	virtual void Tick(float DeltaTime) override;

	/** Cosmetic, call on any machine: a drop of Size (cm) on the floor at FloorLocation. */
	static void AddDrop(UWorld* World, const FVector& FloorLocation, float Size);

private:
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Drops;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DropMID;

	struct FDrop
	{
		FVector Location = FVector::ZeroVector;
		float Size = 0.f;
		float Age = 0.f;
		bool bAlive = false;
	};

	TArray<FDrop> Pool;
	int32 NextDrop = 0;
	int32 AliveDrops = 0;

	void Add(const FVector& FloorLocation, float Size);
	void Init();
};
