#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"
#include "CrvHitProxy.h"

struct FCrvDebugVisualizerCache
{
	void FillReferences(UObject* InTarget, const UActorComponent* InComponent, bool bIsOutgoing);
	void FillCache(UObject* InTarget, const UActorComponent* InComponent, bool bMultiple);
	void Invalidate();

	TSet<TWeakObjectPtr<UObject>> Outgoing;
	TSet<TWeakObjectPtr<UObject>> Incoming;
	TWeakObjectPtr<UObject> Target;
	TWeakObjectPtr<const UActorComponent> Component;
	bool bCached = false;
	bool bHadValidItems = false;

	TSet<UObject*> GetValidCached(bool bIsOutgoing);
};

class CTRLREFERENCEVISUALIZER_API FCrvDebugVisualizer : public FComponentVisualizer
{
public:
	virtual bool VisProxyHandleClick(
		FEditorViewportClient* InViewportClient,
		HComponentVisProxy* VisProxy,
		const FViewportClick& Click
	) override;

private:
	static void InvalidateCache();
	// override from FComponentVisualizer
	virtual void DrawVisualization(
		const UActorComponent* Component,
		const FSceneView* View,
		FPrimitiveDrawInterface* PDI
	) override;

public:
	virtual void OnRegister() override;

	virtual TSharedPtr<SWidget> GenerateContextMenu() const override;

private:
	// void FillCache() const;
	void DrawVisualizationForComponent(
		UObject* Target,
		const UActorComponent* Component,
		FPrimitiveDrawInterface* PDI
	) const;
	static FVector GetComponentLocation(const UActorComponent* Component);
	void DrawLinks(FPrimitiveDrawInterface* PDI, const UActorComponent* Component, bool bIsOutgoing) const;

	// draw arrow between two points
	void DrawDebugLink(
		FPrimitiveDrawInterface* PDI,
		const FVector& SrcOrigin,
		const FVector& DstOrigin,
		const FLinearColor& Color
	) const;

public:
	virtual TSharedPtr<SWidget> MakeContextMenu() const;

private:
	// draw object bounds visualization
	void DrawDebugTarget(FPrimitiveDrawInterface* PDI, const FVector& Location, const FLinearColor& Color) const;

	HCrvHitProxy* LastHitProxy = nullptr;

	static FVector GetActorOrigin(const AActor* Actor);

protected:
	inline static FCrvDebugVisualizerCache Cache;
	inline static bool bMultiple = false;
};
