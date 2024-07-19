// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CrvRefCache.h"
#include "CrvSettings.h"
#include "DebugRenderSceneProxy.h"
#include "Components/ActorComponent.h"
#include "Debug/DebugDrawComponent.h"
#include "Templates/TypeHash.h"
#include "ReferenceVisualizerComponent.generated.h"

class UReferenceVisualizerComponent;
class UCrvRefCache;

UCLASS()
class UReferenceVisualizerEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UReferenceVisualizerEditorSubsystem();

	UPROPERTY(Transient)
	TObjectPtr<UCrvRefCache> Cache;
	UPROPERTY(Transient)
	TObjectPtr<UCrvRefCache> MenuCache;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void UpdateCache();

	void OnObjectModified(UObject* Object);
	void OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	void OnSettingsModified(UObject* Object, FProperty* Property);
	void OnSelectionChanged(UObject* SelectionObject);

private:
	bool bIsRefreshingSelection = false;
	FDelegateHandle SettingsModifiedHandle;
	FTimerHandle UpdateCacheNextTickHandle;
};

USTRUCT()
struct FCrvLine
{
	GENERATED_BODY()
	UPROPERTY()
	FVector Start;
	UPROPERTY()
	FVector End;
	UPROPERTY()
	FLinearColor Color;
	UPROPERTY()
	FCrvLineStyle Style;

	friend bool operator==(const FCrvLine& Lhs, const FCrvLine& RHS)
	{
		return Lhs.Start == RHS.Start
			&& Lhs.End == RHS.End
			&& Lhs.Color == RHS.Color
			&& Lhs.Style == RHS.Style;
	}

	friend bool operator!=(const FCrvLine& Lhs, const FCrvLine& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FCrvLine& Arg)
	{
		uint32 Hash = HashCombine(GetTypeHash(Arg.Start), GetTypeHash(Arg.End));
		Hash = HashCombine(Hash, GetTypeHash(Arg.Color));
		Hash = HashCombine(Hash, GetTypeHash(Arg.Style));
		return Hash;
	}
};

/**
 * Component used to visualize references in the editor.
 * This component is hidden and not blueprintable.
 * This component is added/removed automatically on selection, to trigger the visualizer.
 */
UCLASS(ClassGroup = Debug, NotBlueprintable, NotBlueprintType, NotEditInlineNew, meta = (BlueprintSpawnableComponent))
class CTRLREFERENCEVISUALIZER_API UReferenceVisualizerComponent : public UDebugDrawComponent
{
	GENERATED_BODY()

public:
	virtual FDebugRenderSceneProxy* CreateDebugSceneProxy() override;

	FCrvLine CreateLine(const FVector& SrcOrigin, const FVector& DstOrigin, ECrvDirection Direction, const UClass* Type);

	void CreateLines(const UObject* RootObject, ECrvDirection Direction);

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	UPROPERTY(Transient)
	TObjectPtr<UReferenceVisualizerEditorSubsystem> CrvEditorSubsystem;

	UPROPERTY(Transient)
	TSet<FCrvLine> Lines;

	UReferenceVisualizerComponent();
};

class FCtrlReferenceVisualizerSceneProxy : public FDebugRenderSceneProxy
{
public:
	static FVector GetComponentLocation(const UActorComponent* Component);
	static FVector GetActorOrigin(const AActor* Actor);

	FCtrlReferenceVisualizerSceneProxy(const UPrimitiveComponent* InComponent);

	virtual SIZE_T GetTypeHash() const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual uint32 GetMemoryFootprint() const override;

	virtual void GetDynamicMeshElementsForView(
		const FSceneView* View,
		int32 ViewIndex,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector& Collector,
		FMaterialCache& DefaultMaterialCache,
		FMaterialCache& SolidMeshMaterialCache
	) const override;

	void DrawLines(TSet<FCrvLine> InLines);

	static FVector GetObjectLocation(const UObject* Object);

private:
	ESceneDepthPriorityGroup DepthPriorityGroup = SDPG_World;

protected:
	inline static bool bMultiple = false;
};
