// Chempunk burst effect: glowing chemical splash, expanding ring, rising bubbles and a light flash.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeChemFX.generated.h"

class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UPointLightComponent;
class UStaticMeshComponent;

/** Look of one burst. Set on the GameplayCue Blueprints (GC_Chem_*). */
USTRUCT(BlueprintType)
struct FRougeChemFXSettings
{
	GENERATED_BODY()

	/** Glow color (HDR: values above 1 bloom). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX")
	FLinearColor Color = FLinearColor(0.35f, 1.f, 0.12f, 1.f);

	/** Used instead of Color when the cue's NormalizedMagnitude is 1 (e.g. a critical hit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX")
	FLinearColor StrongColor = FLinearColor(0.75f, 0.2f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "0.0"))
	float Intensity = 4.f;

	/** Size of the splash. The cue's RawMagnitude replaces it when > 0 (skill radius). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "5.0"))
	float Radius = 60.f;

	/** NormalizedMagnitude 1 multiplies the radius by this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "1.0"))
	float StrongScale = 1.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "0.05"))
	float Duration = 0.45f;

	/** Glowing sphere that pops and fades. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX")
	bool bFlash = true;

	/** Flat shockwave ring on the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX")
	bool bRing = false;

	/** Toxic bubbles rising and popping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "0", ClampMax = "32"))
	int32 BubbleCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "1.0"))
	float BubbleSize = 10.f;

	/** How high bubbles rise over the effect's life. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "0.0"))
	float BubbleRise = 120.f;

	/** Point light flash (0 = none; keep it for the big moments, lights cost the most). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "0.0"))
	float LightIntensity = 0.f;

	/** Optional extra Niagara (steam puff, sparks...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX")
	TObjectPtr<UNiagaraSystem> ExtraVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chem FX", meta = (ClampMin = "0.01"))
	float ExtraVFXScale = 0.5f;
};

/**
 * Local only (never replicated): GameplayCue notifies spawn one on every machine. Built from engine shapes and
 * M_ChemGlow (additive HDR glow with a fresnel rim), animated in Tick, destroys itself.
 */
UCLASS(NotBlueprintable)
class ROUGE_API ARougeChemFX : public AActor
{
	GENERATED_BODY()

public:
	ARougeChemFX();

	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** bStrong: crit / big version. RadiusOverride > 0 replaces Settings.Radius. Skips when too many are alive. */
	static ARougeChemFX* SpawnBurst(UWorld* World, const FVector& Location, const FRougeChemFXSettings& Settings,
		bool bStrong = false, float RadiusOverride = 0.f);

private:
	struct FBubble
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		TObjectPtr<UMaterialInstanceDynamic> MID;
		FVector Start = FVector::ZeroVector;
		FVector Drift = FVector::ZeroVector;
		float Delay = 0.f;
		float Size = 1.f;
	};

	void Setup(const FRougeChemFXSettings& InSettings, bool bStrong, float RadiusOverride);
	UStaticMeshComponent* MakeMesh(UStaticMesh* Mesh, UMaterialInstanceDynamic*& OutMID, float RimAmount);
	void SetGlow(UMaterialInstanceDynamic* MID, float Opacity) const;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FlashMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> RingMesh;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FlashMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	TArray<FBubble> Bubbles;

	FLinearColor Color = FLinearColor::Green;
	float Intensity = 4.f;
	float Radius = 60.f;
	float Duration = 0.45f;
	float LightPeak = 0.f;
	float BubbleRise = 120.f;
	/** Ring height relative to the actor (on the floor under it). */
	float RingZ = 2.f;
	float Age = 0.f;

	static int32 AliveCount;
};
