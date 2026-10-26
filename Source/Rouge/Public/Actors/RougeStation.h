// Purification station: the package is docked here and purified while the team defends it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeStation.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class ERougeStationState : uint8
{
	/** Waiting for its turn. */
	Idle,
	/** Chosen: bring the package here. */
	Active,
	/** Package docked (or was), wave on: the bar fills while the package sits here. */
	Purifying,
	/** Done. */
	Purified
};

/**
 * Place the stations (and one with bIsFinal) in the level. The game mode picks the next unpurified station at
 * random, docks the package when a player brings it inside Radius, runs that station's wave and fills Progress
 * while the package stays docked. Station done = bar full and wave cleared; the final one wins the run.
 * Server drives State / Progress; clients only draw (pedestal, glowing circle, progress ring, beacon).
 */
UCLASS()
class ROUGE_API ARougeStation : public AActor
{
	GENERATED_BODY()

public:
	ARougeStation();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Every station in the world (the final one included). */
	static TArray<ARougeStation*> FindAll(const UWorld* World);

	UFUNCTION(BlueprintPure, Category = "Station")
	bool IsFinal() const { return bIsFinal; }

	UFUNCTION(BlueprintPure, Category = "Station")
	ERougeStationState GetState() const { return State; }

	/** 0..1 */
	UFUNCTION(BlueprintPure, Category = "Station")
	float GetProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "Station")
	float GetRadius() const { return Radius; }

	UFUNCTION(BlueprintPure, Category = "Station")
	float GetPurifyDuration() const { return PurifyDuration; }

	/** 1..N in the order the game mode numbered them (0 for the final). */
	UFUNCTION(BlueprintPure, Category = "Station")
	int32 GetStationNumber() const { return StationNumber; }

	/** Where a docked package sits. */
	FVector GetDockLocation() const;

	// Server (game mode).
	void SetState(ERougeStationState NewState);
	void SetProgress(float NewProgress);
	void SetStationNumber(int32 Number) { StationNumber = Number; }
	void SetIsFinal(bool bFinal) { bIsFinal = bFinal; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UStaticMeshComponent> PedestalMesh;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UStaticMeshComponent> CircleMesh;

	/** Grows from the center with the purification progress. */
	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UStaticMeshComponent> ProgressMesh;

	/** Light beam so the active station can be seen from far away. */
	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UStaticMeshComponent> BeaconMesh;

	/** The final station (center of the city): completing it wins the run. */
	UPROPERTY(EditAnywhere, Replicated, Category = "Station")
	bool bIsFinal = false;

	UPROPERTY(EditAnywhere, Category = "Station", meta = (ClampMin = "50.0"))
	float Radius = 320.f;

	/** Seconds the package has to stay docked. */
	UPROPERTY(EditAnywhere, Category = "Station", meta = (ClampMin = "1.0"))
	float PurifyDuration = 35.f;

	UPROPERTY(EditAnywhere, Category = "Station|Look")
	FLinearColor IdleColor = FLinearColor(0.5f, 0.5f, 0.55f, 0.15f);

	UPROPERTY(EditAnywhere, Category = "Station|Look")
	FLinearColor ActiveColor = FLinearColor(1.f, 0.6f, 0.1f, 0.3f);

	UPROPERTY(EditAnywhere, Category = "Station|Look")
	FLinearColor PurifyColor = FLinearColor(0.3f, 1.f, 0.2f, 0.35f);

	UPROPERTY(EditAnywhere, Category = "Station|Look")
	FLinearColor PurifiedColor = FLinearColor(0.2f, 0.8f, 1.f, 0.2f);

private:
	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Station|Runtime")
	ERougeStationState State = ERougeStationState::Idle;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Station|Runtime")
	float Progress = 0.f;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Station|Runtime")
	int32 StationNumber = 0;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CircleMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ProgressMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeaconMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PedestalMID;

	float Age = 0.f;

	/** Cosmetic purification steam / sparks (every machine). */
	float SteamTimer = 0.f;
	float LastFxProgress = 0.f;

	void ApplySize();
	FLinearColor GetStateColor() const;
	void TickPurifyFX(float DeltaTime);
};
