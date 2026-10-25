#include "AbilitySystem/Cues/RougeChemCueNotify.h"

#include "Engine/World.h"

bool URougeChemCueNotify::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UWorld* World = MyTarget ? MyTarget->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	const FVector Location = Parameters.Location.IsNearlyZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
	const bool bStrong = Parameters.NormalizedMagnitude >= 0.5f;
	ARougeChemFX::SpawnBurst(World, Location, Settings, bStrong, Parameters.RawMagnitude);
	return true;
}
