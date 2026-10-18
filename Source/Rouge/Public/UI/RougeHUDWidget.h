// Bottom HUD for the local player (portrait, Q/E skills, HP / shield / team XP bars, collapsible stats panel).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "RougeHUDWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UWidget;
class ARougeCharacter;
class UAbilitySystemComponent;
class URougeSkillAbility;

/** Widgets of one skill slot on the HUD. */
USTRUCT()
struct FRougeHUDSkillSlot
{
	GENERATED_BODY()

	FGameplayTag InputTag;

	/** Skill the name / icon were last filled from. */
	TWeakObjectPtr<const URougeSkillAbility> BoundSkill;

	UPROPERTY()
	TObjectPtr<UBorder> Frame;

	UPROPERTY()
	TObjectPtr<UImage> Icon;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	/** Dark fill over the slot, shrinking as the cooldown runs out. */
	UPROPERTY()
	TObjectPtr<UProgressBar> CooldownShade;

	UPROPERTY()
	TObjectPtr<UTextBlock> CooldownText;
};

/**
 * League-style bottom HUD, built in code (no widget blueprint needed).
 * Reads everything from the owning player's pawn every frame: attributes from the ASC, skill cooldowns from the
 * Q/E ability instances and team XP from the GameState, so it works the same on the listen host and on clients.
 */
UCLASS()
class ROUGE_API URougeHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleStatsPanel();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetStatsPanelOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsStatsPanelOpen() const { return bStatsPanelOpen; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** How often the stats panel text is rebuilt. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.02"))
	float StatsRefreshInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	bool bStatsPanelOpenByDefault = true;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor PanelColor = FLinearColor(0.02f, 0.025f, 0.035f, 0.88f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor FrameColor = FLinearColor(0.78f, 0.62f, 0.3f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor HealthColor = FLinearColor(0.12f, 0.75f, 0.2f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor ShieldColor = FLinearColor(0.3f, 0.75f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor XPColor = FLinearColor(0.55f, 0.3f, 0.95f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor FighterColor = FLinearColor(0.85f, 0.15f, 0.12f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor RangedColor = FLinearColor(0.2f, 0.75f, 0.25f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Colors")
	FLinearColor TankColor = FLinearColor(0.55f, 0.25f, 0.85f, 1.f);

private:
	// ---------------- Layout ----------------

	void BuildLayout();
	UWidget* BuildStatsPanel();
	UWidget* BuildStatsToggle();
	UWidget* BuildPortrait();
	UWidget* BuildMainPanel();
	UWidget* BuildSkillSlot(FRougeHUDSkillSlot& SkillSlot, const FGameplayTag& InputTag, const FString& KeyLabel);
	UWidget* BuildAimSlot();
	UWidget* BuildBar(TObjectPtr<UProgressBar>& OutBar, TObjectPtr<UTextBlock>& OutText, const FLinearColor& FillColor, float Height, int32 FontSize);

	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color = FLinearColor::White, bool bBold = true);
	void AddStatRow(const FText& Label, const FText& Value);

	// ---------------- Updates ----------------

	void RefreshVitals(const UAbilitySystemComponent* ASC);
	void RefreshTeamXP();
	void RefreshSkills(UAbilitySystemComponent* ASC);
	void RefreshSkillSlot(FRougeHUDSkillSlot& SkillSlot, const URougeSkillAbility* Skill);
	void RefreshClassVisuals(const ARougeCharacter* RougeCharacter);
	void RefreshAimSlot(const ARougeCharacter* RougeCharacter);
	void RefreshStatsPanel(const ARougeCharacter* RougeCharacter);
	void ApplyStatsPanelVisibility();

	UFUNCTION()
	void HandleStatsToggleClicked();

	// ---------------- Widgets ----------------

	UPROPERTY()
	TObjectPtr<UWidget> StatsPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> StatsRows;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatsToggleText;

	UPROPERTY()
	TObjectPtr<UBorder> PortraitCircle;

	UPROPERTY()
	TObjectPtr<UTextBlock> PortraitText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ClassNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY()
	TObjectPtr<UWidget> ShieldRow;

	UPROPERTY()
	TObjectPtr<UProgressBar> ShieldBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> ShieldText;

	UPROPERTY()
	TObjectPtr<UProgressBar> XPBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> XPText;

	UPROPERTY()
	TObjectPtr<UBorder> AimFrame;

	UPROPERTY()
	TObjectPtr<UTextBlock> AimText;

	UPROPERTY()
	FRougeHUDSkillSlot QSkillSlot;

	UPROPERTY()
	FRougeHUDSkillSlot ESkillSlot;

	// ---------------- State ----------------

	bool bStatsPanelOpen = true;
	float StatsRefreshCountdown = 0.f;
	/** Highest shield value since the shield came up (the shield bar is shown relative to it). */
	float ShieldPeak = 0.f;
	FGameplayTag LastClassTag;
	bool bClassVisualsApplied = false;

	/** Aim mode the aim slot shows (-1 = not drawn yet). */
	int8 ShownAutoAim = -1;
};
