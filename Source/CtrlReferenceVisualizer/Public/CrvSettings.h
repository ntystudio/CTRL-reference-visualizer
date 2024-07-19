#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "Templates/TypeHash.h"
#include "UObject/ReferenceChainSearch.h"

#include "CrvSettings.generated.h"

UENUM()
enum class ECrvDirection: uint8
{
	Incoming,
	Outgoing,
};

UENUM()
enum class ECrvMode: uint8
{
	OnlySelected, // Only show references to/from the selected actor or component
	SelectedOrAll, // Show references to/from the selected actor or component, or all references if nothing is selected
	All, // Show all references
};

UENUM()
enum class ECrvLineType: uint8
{
	Dash,
	Arrow,
};

USTRUCT()
struct FCrvLineStyle
{
	GENERATED_BODY()

	FCrvLineStyle() = default;

	FCrvLineStyle(
		const ECrvLineType LineType,
		const FLinearColor LineColor,
		const FLinearColor LineColorComponent,
		const FLinearColor LineColorObject,
		const float LineThickness,
		const float ArrowSize,
		const ESceneDepthPriorityGroup DepthPriority
	)
		: LineType(LineType),
		LineColor(LineColor),
		LineColorComponent(LineColorComponent),
		LineColorObject(LineColorObject),
		LineThickness(LineThickness),
		ArrowSize(ArrowSize),
		DepthPriority(DepthPriority) {}

	UPROPERTY(Config, EditAnywhere, Category = "Style | General")
	ECrvLineType LineType = ECrvLineType::Arrow;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", DisplayName = "Line Color")
	FLinearColor LineColor = FLinearColor::Green;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", DisplayName = "Line Color (Component)")
	FLinearColor LineColorComponent = FColor::Emerald;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", DisplayName = "Line Color (Object)")
	FLinearColor LineColorObject = FColor::Cyan;
	
	UPROPERTY(Config, EditAnywhere, Category = "Style | General", meta = (ClampMin = "0", UIMin = "0", UIMax = "10", EditCondition = "LineType != ECrvLineStyle::Dash"))
	float LineThickness = 2.f;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General", meta = (ClampMin = "0", UIMin = "0", UIMax = "100", EditCondition = "LineType == ECrvLineStyle::Arrow"))
	float ArrowSize = 10.f;

	UPROPERTY(Config, EditAnywhere, Category = "Style | General")
	TEnumAsByte<ESceneDepthPriorityGroup> DepthPriority = ESceneDepthPriorityGroup::SDPG_Foreground;

	friend bool operator==(const FCrvLineStyle& Lhs, const FCrvLineStyle& RHS)
	{
		return Lhs.LineType == RHS.LineType
			&& Lhs.LineColor == RHS.LineColor
			&& Lhs.LineColorComponent == RHS.LineColorComponent
			&& Lhs.LineColorObject == RHS.LineColorObject
			&& Lhs.LineThickness == RHS.LineThickness
			&& Lhs.ArrowSize == RHS.ArrowSize
			&& Lhs.DepthPriority == RHS.DepthPriority;
	}

	friend bool operator!=(const FCrvLineStyle& Lhs, const FCrvLineStyle& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FCrvLineStyle& Arg)
	{
		uint32 Hash = HashCombine(GetTypeHash(Arg.LineType), GetTypeHash(Arg.LineColor));
		Hash = HashCombine(Hash, GetTypeHash(Arg.LineColorComponent));
		Hash = HashCombine(Hash, GetTypeHash(Arg.LineColorObject));
		Hash = HashCombine(Hash, GetTypeHash(Arg.LineThickness));
		Hash = HashCombine(Hash, GetTypeHash(Arg.ArrowSize));
		Hash = HashCombine(Hash, GetTypeHash(Arg.DepthPriority));
		return Hash;
	}

	static FCrvLineStyle DefaultOutgoingLineStyle;

	static FCrvLineStyle DefaultIncomingLineStyle;
};


USTRUCT()
struct FCrvStyleSettings
{
	GENERATED_BODY()

	FCrvStyleSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Style | General")
	FCrvLineStyle LineStyleOutgoing = FCrvLineStyle::DefaultOutgoingLineStyle;
	
	UPROPERTY(Config, EditAnywhere, Category = "Style | General")
	FCrvLineStyle LineStyleIncoming = FCrvLineStyle::DefaultIncomingLineStyle;

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

	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Style | Target Circles",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "10", EditCondition = "bDrawTargetCircles")
	)
	float CircleLineThickness = 1.f;
	/* Circle around the currently selected actor or scene component */
	UPROPERTY(Config, EditAnywhere, Category = "Style | Target Circles", meta = (EditCondition = "bDrawTargetCircles"))
	FLinearColor CurrentCircleColor = FLinearColor::Gray;

	/* Circle around referenced actors or scene components */
	UPROPERTY(Config, EditAnywhere, Category = "Style | Target Circles", meta = (EditCondition = "bDrawTargetCircles"))
	FLinearColor LinkedCircleColor = FLinearColor::Transparent;
};

USTRUCT()
struct FCrvTargetSettings
{
	explicit FCrvTargetSettings();

	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Targets")
	TArray<TSoftClassPtr<UObject>> IgnoreReferencesToClasses;

	/* The target actor component classes to enable with the reference viewer debug visualization. */
	UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Targets")
	TArray<TSoftClassPtr<UActorComponent>> TargetComponentClasses;
};

UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EReferenceChainSearchMode_K2: uint32
{
	// Returns all reference chains found
	Default = 0,
	// Returns only reference chains from external objects
	ExternalOnly = 1 << 0,
	// Returns only the shortest reference chain for each rooted object
	Shortest = 1 << 1,
	// Returns only the longest reference chain for each rooted object
	Longest = 1 << 2,
	// Returns only the direct referencers
	Direct = 1 << 3,
	// Returns complete chains. (Ignoring non GC objects)
	FullChain = 1 << 4,
	// Returns the shortest path to a garbage object from which the target object is reachable
	ShortestToGarbage = 1 << 5,
	// Attempts to find a plausible path to the target object with minimal memory usage
	// E.g. returns a direct external reference to the target object if one is found 
	//	otherwise returns an external reference to an inner of the target object
	Minimal = 1 << 6,
	// Skips the disregard-for-GC set that will never be GCd and whose outgoing references are not checked during GC
	GCOnly = 1 << 7,

	// Print results
	PrintResults = 1 << 16,
	// Print ALL results (in some cases there may be thousands of reference chains)
	PrintAllResults = 1 << 17,
};

namespace CtrlRefViz
{
	inline FString LexToString(const EReferenceChainSearchMode_K2 Mode)
	{
		return UEnum::GetValueOrBitfieldAsString(Mode);
	}

	inline FString LexToString(EReferenceChainSearchMode Mode)
	{
		return LexToString(static_cast<EReferenceChainSearchMode_K2>(Mode));
	}
}

/**
 * 
 */
UCLASS(
	Config = EditorPerProjectUserSettings,
	PerObjectConfig,
	meta = (DisplayName = "Ctrl - Reference Visualizer Settings")
)
class CTRLREFERENCEVISUALIZER_API UCrvSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UCrvSettings();

	virtual FName GetContainerName() const override
	{
		return FName("Editor");
	}

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

	bool IsEnabled() const;
	bool GetAutoAddComponents() const;

	/* Whether the reference viewer display is enabled */
	UPROPERTY(Config, EditAnywhere, Category = "General", meta = (ConsoleVariable = "ctrl.ReferenceVisualizer"))
	bool bIsEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "General", meta=(MultiLine=true))
	FText Help;

	UPROPERTY(Config, EditAnywhere, Category = "General")
	ECrvMode Mode = ECrvMode::SelectedOrAll;

	// automatically add/remove ReferenceVisualizerComponent to actors
	UPROPERTY(Config, EditAnywhere, Category = "General")
	bool bAutoAddComponents = true;

	// show recursive references to/from the selected actor or component
	// UPROPERTY(Config, EditAnywhere, Category = "General", meta = (ClampMin = "1", UIMin = "1", UIMax = "100", EditCondition = "Mode == ECrvMode::OnlySelected || Mode == ECrvMode::SelectedOrAll"))
	// int32 Depth = 1;

	UFUNCTION(BlueprintCallable, Exec, CallInEditor)
	void Documentation() const;

	UPROPERTY(Config, EditAnywhere, Category = "General", DisplayName = "Visualize Outgoing References")
	bool bShowOutgoingReferences = true;

	UPROPERTY(Config, EditAnywhere, Category = "General", DisplayName = "Visualize Incoming References (Potentially Slow)")
	bool bShowIncomingReferences = true;

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
	FCrvStyleSettings Style;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (ShowOnlyInnerProperties))
	FCrvTargetSettings TargetSettings;
	/* Include recursive references to child actors and components */
	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bIsRecursive = true;
	// /* Include references from parent actor, rather than just the selected component */
	// UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	// bool bAlwaysIncludeReferencesFromParent = false;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bShowComponents = true;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bShowObjects = false;

	// UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (EditCondition = "bIsRecursive"))
	// bool bWalkComponents = true;
	//
	// UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (EditCondition = "bIsRecursive"))
	// bool bWalkSceneChildComponents = true;
	//
	// UPROPERTY(Config, EditAnywhere, Category = "General|Filtering", meta = (EditCondition = "bIsRecursive"))
	// bool bWalkChildActors = true;

	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bWalkObjectProperties = true;

	/* Skip ObjectArchetype references */
	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bIgnoreArchetype = true;

	/* Skip transient properties */
	UPROPERTY(Config, EditAnywhere, Category = "General|Filtering")
	bool bIgnoreTransient = true;

	UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "General")
	bool bDebugEnabled = false;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "General")
	bool bRefreshEnabled = true;

	/* Delegate called when this settings object is modified */
	DECLARE_MULTICAST_DELEGATE_TwoParams(UCrvSettingsModified, UObject*, FProperty*);
	UCrvSettingsModified OnModified;

	FCrvLineStyle GetLineStyle(ECrvDirection Direction) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void CVarSink();
#endif
	virtual void PostInitProperties() override;

private:
	bool bIsUpdating = false;
};
