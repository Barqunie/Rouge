// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Animation/RougeArmedAnimSet.h"
#include "CharacterBase.generated.h"

class UAnimSequenceBase;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USkeletalMesh;
class USkeletalMeshComponent;
class UIKRetargeter;
class UStaticMesh;
class UStaticMeshComponent;

/** A static mesh (weapon, prop...) held by the visible mesh. */
USTRUCT(BlueprintType)
struct FRougeVisualAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	TObjectPtr<UStaticMesh> Mesh;

	/** Bone or socket of the visible mesh (e.g. hand_r, weapon_r). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	FName Socket;

	/** Relative to the socket: turn the grip / blade direction here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rouge|Visual")
	FTransform Offset;
};

UCLASS()
class ROUGE_API ACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:

	ACharacterBase();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent)
	void OnXPChanged(float CurrentXP, float MaxXP);

	UFUNCTION(BlueprintPure, Category = "Rouge|Character Info")
	FGameplayTag GetCharacterTag() const { return CharacterTag; }

	/** Server only. Called by the attribute set when Health reaches 0. */
	virtual void HandleHealthDepleted() {}

	UFUNCTION(BlueprintPure, Category = "Rouge|Character Info")
	virtual bool IsDead() const { return false; }

	/**
	 * Cosmetic, every machine (called by the hit GameplayCue): glow flash, squash, a short animation freeze
	 * and a hit react animation. bStrong = critical hit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rouge|Hit Feel")
	virtual void PlayHitReaction(bool bStrong);

	virtual void Tick(float DeltaTime) override;

	/** The mesh players see: VisualMesh when a retargeted look is set, else the character mesh. */
	UFUNCTION(BlueprintPure, Category = "Rouge|Character Info")
	USkeletalMeshComponent* GetVisibleMesh() const;

	FVector GetCarriedPackageOffset() const { return CarriedPackageOffset; }

	/**
	 * Toxic parts turn purple while carrying the package (materials with a CarrySwap scalar). Local look, called on
	 * every machine that draws; fades in / out over CarryLookFadeTime.
	 */
	void SetCarryLook(bool bCarrying);

protected:

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();
	virtual void BindCallbacksToDependencies();
	virtual void InitClassDefaults();

	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValues();

	UPROPERTY(EditAnywhere, Category = "Rouge|Character Info")
	FGameplayTag CharacterTag;

	/**
	 * Optional look with its own skeleton (a custom model). Drawn instead of Mesh: Mesh stays hidden but keeps running
	 * the anim blueprint, montages and hit reacts, and its pose is retargeted onto this every frame with RetargetAsset.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Visual")
	TObjectPtr<USkeletalMesh> RetargetedMesh;

	/** IK Retargeter from Mesh's skeleton (source) to RetargetedMesh's skeleton (target). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Visual")
	TObjectPtr<UIKRetargeter> RetargetAsset;

	UPROPERTY(VisibleAnywhere, Category = "Rouge|Visual")
	TObjectPtr<USkeletalMeshComponent> VisualMesh;

	/**
	 * Optional clips of RetargetedMesh's own skeleton (e.g. holding a two-handed weapon). When set, the look plays
	 * these for idle / walk / shooting and only retargets the character mesh for death, revive and skills.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Visual")
	FRougeArmedAnimSet ArmedAnims;

	/** Props held by the visible mesh (VisualMesh or Mesh), created on every machine that draws. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Visual")
	TArray<FRougeVisualAttachment> VisualAttachments;

	/**
	 * Extra push for a carried package, in the character's frame (X forward, Y right, Z up). The package sits on
	 * Mesh's PackageSocket; a bulkier retargeted look (backpack, tank...) can move it out of the body.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Visual")
	FVector CarriedPackageOffset = FVector::ZeroVector;

	/** Glow flash over the mesh when hit (overlay material). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel")
	FLinearColor HitFlashColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel")
	FLinearColor StrongHitFlashColor = FLinearColor(0.8f, 0.3f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel", meta = (ClampMin = "0.0"))
	float HitFlashTime = 0.1f;

	/** Squash & stretch amount (0.12 = 12% wider and flatter for a moment). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float HitSquash = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel", meta = (ClampMin = "0.01"))
	float HitSquashTime = 0.14f;

	/** Animation freeze on hit ("hit stop"), seconds. Crits freeze a bit longer. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel", meta = (ClampMin = "0.0"))
	float HitPauseTime = 0.f;

	/** Played in the mesh's DefaultSlot when hit (one at random). Empty = no animation. */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel")
	TArray<TObjectPtr<UAnimSequenceBase>> HitReactAnims;

	/** Crit version (falls back to HitReactAnims). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel")
	TArray<TObjectPtr<UAnimSequenceBase>> StrongHitReactAnims;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Hit Feel", meta = (ClampMin = "0.1"))
	float HitReactPlayRate = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Visual", meta = (ClampMin = "0.0"))
	float CarryLookFadeTime = 0.35f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HitFlashMID;

	float HitFlashRemaining = 0.f;
	float CarryLookAlpha = 0.f;
	float CarryLookTarget = 0.f;
	float HitSquashElapsed = -1.f;
	float HitPauseRemaining = 0.f;
	float HitStrength = 1.f;
	FVector BaseMeshScale = FVector::ZeroVector;

	void UpdateHitReaction(float DeltaTime);
	void SetupRetargetedLook();
	void SpawnVisualAttachments();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> VisualAttachmentComponents;
};
