#include "AbilitySystem/Cues/RougeChemCueNotify.h"

#include "Character/RougeCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

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

	if (bPlayHitReaction)
	{
		if (ACharacterBase* HitCharacter = Cast<ACharacterBase>(MyTarget))
		{
			HitCharacter->PlayHitReaction(bStrong);
		}
	}

	// Shake the local player's camera when this went off near them.
	if (CameraShake > 0.f)
	{
		const float Radius = Parameters.RawMagnitude > 0.f ? Parameters.RawMagnitude : Settings.Radius;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PlayerController = It->Get();
			ARougeCharacter* LocalCharacter = PlayerController && PlayerController->IsLocalController()
				? PlayerController->GetPawn<ARougeCharacter>() : nullptr;
			if (LocalCharacter)
			{
				const float Distance = FVector::Dist2D(LocalCharacter->GetActorLocation(), Location);
				const float Falloff = 1.f - FMath::Clamp((Distance - Radius) / FMath::Max(CameraShakeRange, 1.f), 0.f, 1.f);
				if (Falloff > 0.f)
				{
					LocalCharacter->AddCameraTrauma(CameraShake * Falloff * (bStrong ? 1.4f : 1.f));
				}
			}
		}
	}
	return true;
}
