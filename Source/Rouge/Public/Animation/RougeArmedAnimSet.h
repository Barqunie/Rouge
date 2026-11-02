// Clips a weapon-holding look plays on its own skeleton (see URougeArmedAnimInstance).

#pragma once

#include "CoreMinimal.h"
#include "RougeArmedAnimSet.generated.h"

class UAnimSequenceBase;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FRougeArmedAnimSet
{
	GENERATED_BODY()

	/** Holding the weapon, standing (loop). Leave empty to use plain retargeting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	TObjectPtr<UAnimSequenceBase> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	TObjectPtr<UAnimSequenceBase> Walk;

	/** Shooting, standing (loop). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	TObjectPtr<UAnimSequenceBase> Fire;

	/** Shooting while walking (loop, same feet as Walk). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	TObjectPtr<UAnimSequenceBase> WalkFire;

	/** Ground speed at which Walk / WalkFire play at their normal rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual", meta = (ClampMin = "10.0"))
	float WalkAnimSpeed = 300.f;

	/** Montages on the character mesh that mean "shooting" (the basic attack). Other montages (skills, revive)
	 *  and death fall back to the retargeted pose of the character mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	TArray<TObjectPtr<UAnimMontage>> FireMontages;

	/** Keeps the fire pose this long after the last shot montage, so a stream of attacks reads as one burst. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual", meta = (ClampMin = "0.0"))
	float FireHoldTime = 0.45f;

	bool IsSet() const { return Idle != nullptr; }
};
