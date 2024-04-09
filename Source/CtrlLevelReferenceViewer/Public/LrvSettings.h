#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"

#include "LrvSettings.generated.h"


UENUM()
enum class ELrvLineStyle: uint8
{
	Dash,
	Arrow,
};

USTRUCT()
struct FLrvStyleSettings
{
	GENERATED_BODY()

	FLrvStyleSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Style | General")
	ELrvLineStyle LineStyle = ELrvLineStyle::Arrow;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", DisplayName = "Line Color (Outgoing)")
	FLinearColor LineColor = FLinearColor::Green;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", DisplayName = "Line Color (Incoming)")
	FLinearColor LineColorIncoming = FLinearColor::Blue;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", meta = (ClampMin = "0", UIMin = "0", UIMax = "10", EditCondition = "LineStyle != ELrvLineStyle::Dash"))
	float LineThickness = 2.f;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", meta = (ClampMin = "0", UIMin = "0", UIMax = "100", EditCondition = "LineStyle == ELrvLineStyle::Arrow"))
	float ArrowSize = 10.f;

	/* Draw circles around current target & references. */
	UPROPERTY(Config, EditAnywhere, Category = "Style | Target Circles")
	bool bDrawTargetCircles = false;

	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Style | Target Circles",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "1000", EditCondition = "bDrawTargetCircles")
	)
	float CircleRadius = 50.f;

	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Style | Target Circles",
		meta = (ClampMin = "3", UIMin = "3", UIMax = "100", EditCondition = "bDrawTargetCircles")
	)
	float CircleResolution = 32.f;

	/* Circle around the currently selected actor or scene component */
	UPROPERTY(Config, EditAnywhere, Category = "Style | Target Circles", meta = (EditCondition = "bDrawTargetCircles"))
	FLinearColor CurrentCircleColor = FLinearColor::Gray;

	/* Circle around referenced actors or scene components */
	UPROPERTY(Config, EditAnywhere, Category = "Style | Target Circles", meta = (EditCondition = "bDrawTargetCircles"))
	FLinearColor LinkedCircleColor = FLinearColor::Transparent;
};

USTRUCT()
struct FLrvTargetSettings
{
	explicit FLrvTargetSettings();

	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Targets")
	TArray<TSoftClassPtr<UObject>> IgnoreReferencesToClasses;

	/* The target actor component classes to enable with the reference viewer debug visualization. */
	UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Targets")
	TArray<TSoftClassPtr<UActorComponent>> TargetComponentClasses;
};

/**
 * 
 */
UCLASS(
	Config = EditorPerProjectUserSettings,
	PerObjectConfig,
	meta = (DisplayName = "Ctrl - Level Reference Viewer Settings")
)
class CTRLLEVELREFERENCEVIEWER_API ULrvSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	ULrvSettings();
	virtual FName GetContainerName() const override { return FName("Editor"); }
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	UFUNCTION()
	void AddComponentClass(const TSoftClassPtr<UActorComponent>& ComponentClass);
	void SetEnabled(bool bNewEnabled);
	void ToggleEnabled();
	void SetShowIncomingReferences(bool bNewShowIncomingReferences);
	void SetShowOutgoingReferences(bool bNewShowOutgoingReferences);
	void Refresh();
	void UpdateTargets();

	UFUNCTION()
	void CleanTargets();
	/* Whether the reference viewer display is enabled */
	UPROPERTY(Config, EditAnywhere, Category = "General", meta = (ConsoleVariable = "ctrl.LevelReferenceViewer"))
	bool bIsEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "General", meta=(MultiLine=true))
	FText Help;

	UFUNCTION(BlueprintCallable, Exec, CallInEditor)
	void Documentation() const;

	UPROPERTY(Config, EditAnywhere, Category = "General", DisplayName = "Visualize Outgoing References")
	bool bShowOutgoingReferences = true;

	UPROPERTY(Config, EditAnywhere, Category = "General", DisplayName = "Visualize Incoming References (Slow)")
	bool bShowIncomingReferences = false;

	UPROPERTY(Config, EditAnywhere, Category = "General", DisplayName = "Move Camera to Reference On Select")
	bool bMoveViewportCameraToReference = false;

	/* Use the actor bounds to determine center points, otherwise uses actor/scene component location */
	UPROPERTY(Config, EditAnywhere, Category = "General", DisplayName = "Use Center of Actor Bounds as Target Point", Experimental)
	bool bUseActorBounds = false;

	UPROPERTY(Config, EditAnywhere, Category = "General|Bounds", meta = (EditCondition = "bUseActorBounds"), DisplayName = "Include Non-Colliding Components")
	bool bIncludeNonCollidingBounds = false;

	UPROPERTY(Config, EditAnywhere, Category = "General|Bounds", meta = (EditCondition = "bUseActorBounds"), DisplayName = "Include Child Actors")
	bool bIncludeChildActorsInBounds = false;

	/* Controls the style of the reference viewer debug drawing */
	UPROPERTY(Config, EditAnywhere, Category = "Style", meta = (ShowOnlyInnerProperties))
	FLrvStyleSettings Style;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (ShowOnlyInnerProperties))
	FLrvTargetSettings TargetSettings;
	/* Include recursive references to child actors and components */
	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bIsRecursive = true;
	/* Include references from parent actor, rather than just the selected component */
	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bAlwaysIncludeReferencesFromParent = false;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bShowComponents = true;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bShowObjects = false;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (EditCondition = "bIsRecursive"))
	bool bWalkComponents = true;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (EditCondition = "bIsRecursive"))
	bool bWalkSceneChildComponents = true;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (EditCondition = "bIsRecursive"))
	bool bWalkChildActors = true;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bWalkObjectProperties = false;
	
	/* Skip ObjectArchetype references */
	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bIgnoreArchetype = true;

	/* Skip transient properties */
	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bIgnoreTransient = true;
	
	UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "General")
	bool bDebugEnabled = false;

	/* Delegate called when this settings object is modified */
	DECLARE_MULTICAST_DELEGATE_TwoParams(ULrvSettingsModified, UObject*, FProperty*);
	ULrvSettingsModified OnModified;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void CVarSink();
#endif
	virtual void PostInitProperties() override;

private:
	bool bIsUpdating = false;
};
