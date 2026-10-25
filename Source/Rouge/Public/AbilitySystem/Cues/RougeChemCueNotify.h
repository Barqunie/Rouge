// GameplayCue that plays a chempunk burst (ARougeChemFX).

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Actors/RougeChemFX.h"
#include "RougeChemCueNotify.generated.h"

/**
 * Parent of the GC_Chem_* Blueprints (each sets GameplayCueTag + the look). Runs on every machine through the
 * normal GAS cue replication (ASC ExecuteGameplayCue) or ARougeGameState::MulticastChemCue.
 * Parameters used: Location (else the target's location), RawMagnitude = radius override,
 * NormalizedMagnitude >= 0.5 = strong version (crit).
 */
UCLASS(Blueprintable, meta = (DisplayName = "Rouge Chem Cue"))
class ROUGE_API URougeChemCueNotify : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Chem FX")
	FRougeChemFXSettings Settings;
};
