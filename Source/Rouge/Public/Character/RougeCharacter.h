// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Logging/LogMacros.h"
#include <AbilitySystemInterface.h>
#include "Interfaces/RougeAbilitySystemInterface.h"
#include "Character/RougeCharacterInfo.h"

#include "RougeCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAbilitySystemComponent;
class URougeAbilitySystemComponent;
class URougeAttributeSet;
class UAnimationAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class URougeHUDWidget;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** One row of the HUD stats panel. */
USTRUCT(BlueprintType)
struct FRougeStatLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FText Value;
};


UCLASS(abstract)
class ARougeCharacter : public ACharacterBase, public IAbilitySystemInterface, public IRougeAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaTime) override;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Toggles between mouse aim and auto aim (default: IA_ToggleAimMode, left mouse button in IMC_Combat). */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleAimModeAction;

	///** Look Input Action */
	//UPROPERTY(EditAnywhere, Category="Input")
	//UInputAction* LookAction;

	///** Mouse Look Input Action */
	//UPROPERTY(EditAnywhere, Category="Input")
	//UInputAction* MouseLookAction;


	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;



	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);


	virtual void InitAbilityActorInfo() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void InitClassDefaults() override;
	virtual void BroadcastInitialValues() override;

public:

	/** Constructor */
	ARougeCharacter();	



	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;	
	virtual void NotifyControllerChanged() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//UFUNCTION(Server, Reliable)
	//void ServerSetLookRotation(const FRotator& NewRotation);

	/*Implement RougeAbilitySystemInterface */

	virtual USceneComponent* GetDynamicSpawnPoint_Implementation() override;

	// Unreliable: sent often, a dropped packet is covered by the next one / the periodic resend in Tick.
	UFUNCTION(Server, Unreliable)
	void ServerSetLookRotation(const FRotator& NewRotation);

	/** Player name from the PlayerState (Steam nick when online). Empty until the PlayerState has replicated. */
	UFUNCTION(BlueprintPure, Category = "Player Info")
	FString GetPlayerDisplayName() const;

	/** Server: move faster for Duration seconds (support zone). A stronger buff wins; refreshing extends it. */
	void ApplySpeedBuff(float Multiplier, float Duration);

	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetSpeedBuffMultiplier() const { return SpeedBuffMultiplier; }

	/** Name, role, portrait and ability texts / icons (character select screen and HUD). */
	UFUNCTION(BlueprintPure, Category = "Character Select")
	const FRougeCharacterDisplayInfo& GetDisplayInfo() const { return DisplayInfo; }

	/** Fired on every machine when this pawn's player name becomes known or changes (e.g. to update the overhead widget). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Info")
	void OnPlayerNameChanged(const FString& NewName);

	/** Fires OnPlayerNameChanged with the current name (no-op while the name is unknown). */
	void BroadcastPlayerName();

	// ---------------- Stats panel ----------------

	/** Owning client: formatted stats (level, HP, armor, crit, move speed, shield) for the HUD stats panel. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
	void OnStatsChanged(const FText& StatsText);

	/** Rebuilds the stats text and fires OnStatsChanged (locally controlled characters only). */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RefreshStats();

	/** Stats panel rows (level, HP, armor, crit, move speed, shield). Empty until the attributes exist. */
	UFUNCTION(BlueprintPure, Category = "Stats")
	TArray<FRougeStatLine> GetStatLines() const;

	/** Opens / closes the stats panel next to the HUD portrait (Tab). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleStatsPanel();

	// ---------------- Aim mode ----------------
	// Local (owning client) choice: face the mouse cursor, or auto-face the nearest enemy in range.
	// Either way the resulting yaw reaches the server through ServerSetLookRotation.

	UFUNCTION(BlueprintCallable, Category = "Aim")
	void ToggleAimMode();

	UFUNCTION(BlueprintPure, Category = "Aim")
	bool IsAutoAimEnabled() const { return bAutoAim; }

	/** Owning client: aim mode switched (e.g. to show an indicator). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Aim")
	void OnAimModeChanged(bool bAutoAimEnabled);

	// ---------------- Death / revive ----------------
	// Server: Health 0 -> dead. Revived when RespawnDelay runs out, or after an alive teammate
	// stands inside the revive circle for ReviveDuration (both run in parallel). If the player
	// dies again within TeammateReviveCooldown of a revive, only the timer can bring them back.

	/** Server only. Called by the attribute set when Health reaches 0. */
	virtual void HandleHealthDepleted() override;

	virtual bool IsDead() const override { return bIsDead; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Seconds until the automatic respawn (0 when alive). Valid on every machine. */
	UFUNCTION(BlueprintPure, Category = "Death")
	float GetRespawnTimeRemaining() const;

	/** 0..1 progress of a teammate revive. */
	UFUNCTION(BlueprintPure, Category = "Death")
	float GetReviveProgress() const { return ReviveProgress; }

	/** True when teammates can't revive this player (died again shortly after a revive). */
	UFUNCTION(BlueprintPure, Category = "Death")
	bool IsTeammateReviveBlocked() const { return bTeammateReviveBlocked; }

	/** All machines: fired when this player dies or is revived. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death")
	void OnDeathStateChanged(bool bDead);

	/** All machines, every frame while dead: drive the death timer / revive UI. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death")
	void OnDeathTick(float RespawnTimeRemaining, float CurrentReviveProgress, bool bReviveBlocked);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death", meta = (ClampMin = "0.1"))
	float RespawnDelay = 20.f;

	/** Seconds a teammate must stay inside the circle to revive (progress is kept if they step out). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death", meta = (ClampMin = "0.1"))
	float ReviveDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death", meta = (ClampMin = "10.0"))
	float ReviveRadius = 250.f;

	/** Damage immunity right after being revived. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death", meta = (ClampMin = "0.0"))
	float ReviveImmunityDuration = 1.f;

	/** Dying again within this many seconds of a revive disables teammate revive for that death. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death", meta = (ClampMin = "0.0"))
	float TeammateReviveCooldown = 60.f;

	/** Health restored on revive, as a fraction of MaxHealth. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ReviveHealthFraction = 1.f;

	/** Optional animation played (single node) while dead. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death")
	TObjectPtr<UAnimationAsset> DeathAnimation;

	/** Material for the revive circle. Needs a vector parameter named "Color". */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death")
	TObjectPtr<UMaterialInterface> ReviveCircleMaterial;

	/** Screen saturation for the dead player's own camera (0 = black & white, 1 = normal). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeadScreenSaturation = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death")
	FLinearColor ReviveAreaColor = FLinearColor(0.1f, 0.55f, 1.f, 0.35f);

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death")
	FLinearColor ReviveBlockedColor = FLinearColor(1.f, 0.15f, 0.15f, 0.35f);

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Death")
	FLinearColor ReviveProgressColor = FLinearColor(0.2f, 1.f, 0.35f, 0.6f);

	/** Ground circle showing the revive area (visible while dead). */
	UPROPERTY(VisibleAnywhere, Category = "Death")
	TObjectPtr<UStaticMeshComponent> ReviveAreaMesh;

	/** Inner circle that grows with the revive progress. */
	UPROPERTY(VisibleAnywhere, Category = "Death")
	TObjectPtr<UStaticMeshComponent> ReviveProgressMesh;

	/** Translucent bubble shown while the Shield attribute is above 0 (tank shield skill). */
	UPROPERTY(VisibleAnywhere, Category = "Shield")
	TObjectPtr<UStaticMeshComponent> ShieldBubbleMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Shield")
	FLinearColor ShieldBubbleColor = FLinearColor(0.3f, 0.75f, 1.f, 0.25f);

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ShieldBubbleMID;

	/** All machines: shows / hides the shield bubble (Shield attribute change). */
	void UpdateShieldBubble(float ShieldValue);

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ReviveAreaMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ReviveProgressMID;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	/** Server world time of the automatic respawn. */
	UPROPERTY(Replicated)
	float RespawnServerTime = 0.f;

	UPROPERTY(Replicated)
	float ReviveProgress = 0.f;

	UPROPERTY(Replicated)
	bool bTeammateReviveBlocked = false;

	/** 1 = no buff. Replicated so the owning client predicts movement with the same speed. */
	UPROPERTY(ReplicatedUsing = OnRep_SpeedBuff)
	float SpeedBuffMultiplier = 1.f;

	/** Server: when the speed buff runs out. */
	float SpeedBuffEndTime = 0.f;

	/** MaxWalkSpeed without buffs (from the Blueprint defaults). */
	float BaseWalkSpeed = 0.f;

	UFUNCTION()
	void OnRep_SpeedBuff();

	void ApplyWalkSpeed();
	void UpdateSpeedBuff();

	/** Server only. Server world time of the last revive (-1 = never). */
	float LastReviveServerTime = -1.f;

	FTimerHandle ReviveImmunityTimer;

	UFUNCTION()
	void OnRep_IsDead();

	/** All machines: tags, movement, collision, animation and circle visibility for the current bIsDead. */
	void ApplyDeathState();

	/** All machines, every frame while dead. */
	void UpdateDeathPresentation();

	void TickDeathServer(float DeltaTime);
	void Revive();
	void ClearReviveImmunity();
	bool IsAliveTeammateInReviveRange() const;
	float GetServerWorldTime() const;
	void InitReviveCircleMaterials();

	/** Shows the cursor and puts the local player in game input mode without eating mouse clicks. */
	void ApplyGameInputMode();

	/** Shown on the character select screen and the HUD. Q / E ability rows also give the HUD skill icons. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Select", meta = (AllowPrivateAccess = "true"))
	FRougeCharacterDisplayInfo DisplayInfo;

	/** Bottom HUD (portrait, skills, bars, stats panel), created for the local player. None disables it. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<URougeHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<URougeHUDWidget> HUDWidget;

	/** Runtime Tab -> ToggleStatsPanel binding (owning client). */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleStatsAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> StatsMappingContext;

	void CreateHUDWidget();
	void SetCircleScale(UStaticMeshComponent* Circle, float Radius) const;

	/** Minimum yaw change (degrees) before the owning client sends a new look rotation to the server. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Aim")
	float LookYawSendThreshold = 0.5f;

	/** Resend the current look rotation at this interval even if unchanged, in case an unreliable RPC was dropped. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Aim")
	float LookResendInterval = 0.2f;

	float LastSentLookYaw = 0.f;
	float LastLookSendTime = -1.f;

	/** Auto aim only picks enemies within this distance; otherwise it falls back to the mouse. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Aim", meta = (ClampMin = "100.0"))
	float AutoAimRange = 1500.f;

	/** How often auto aim re-picks the nearest enemy. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Aim", meta = (ClampMin = "0.02"))
	float AutoAimRetargetInterval = 0.15f;

	bool bAutoAim = false;
	TWeakObjectPtr<AActor> AutoAimTarget;
	float NextAutoAimSearchTime = 0.f;

	/** Owning client: nearest alive enemy position in range, if any. */
	bool GetAutoAimPoint(FVector& OutPoint);
	AActor* FindAutoAimTarget() const;

	void SendLookRotationToServer(const FRotator& NewRotation);

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<USceneComponent> DynamicProjectileSpawnPoint;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URougeAbilitySystemComponent> RougeAbilitySystemComp;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URougeAttributeSet> RougeAttributes;











};

