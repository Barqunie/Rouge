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

	/** The cue's target plays its hit reaction (flash, squash, hit stop, hit react animation). */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Feel")
	bool bPlayHitReaction = false;

	/** Camera shake for the local player when the burst is near them (0 = none). */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Feel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CameraShake = 0.f;

	/** "Near" = within the burst radius plus this. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Feel", meta = (ClampMin = "0.0"))
	float CameraShakeRange = 500.f;
};
