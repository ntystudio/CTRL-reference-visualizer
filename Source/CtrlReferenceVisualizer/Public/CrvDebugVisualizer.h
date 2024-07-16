#pragma once

#include "CoreMinimal.h"
#include "CrvHitProxy.h"
#include "CrvRefCache.h"

class CTRLREFERENCEVISUALIZER_API FCrvDebugVisualizer : public FComponentVisualizer
{
public:
	virtual bool VisProxyHandleClick(
		FEditorViewportClient* InViewportClient,
		HComponentVisProxy* VisProxy,
		const FViewportClick& Click
	) override;

private:
	static void InvalidateCache(const FString& Reason);
	// override from FComponentVisualizer
	virtual void DrawVisualization(
		const UActorComponent* RootComponent,
		const FSceneView* View,
		FPrimitiveDrawInterface* PDI
	) override;

public:
	virtual void OnRegister() override;

	virtual TSharedPtr<SWidget> GenerateContextMenu() const override;

private:
	void DrawVisualizationForRootObject(
		UObject* RootObject,
		const UActorComponent* RootComponent,
		FPrimitiveDrawInterface* PDI
	) const;
	static FVector GetComponentLocation(const UActorComponent* Component);

	static void OnHoverProxy(FCrvHitProxyRef HitProxy);

	void DrawLinks(FPrimitiveDrawInterface* PDI, const UActorComponent* RootComponent, const UObject* RootObject, bool bIsOutgoing) const;

	// draw arrow between two points
	void DrawDebugLink(
		FPrimitiveDrawInterface* PDI,
		const FVector& SrcOrigin,
		const FVector& DstOrigin,
		const bool bIsOutgoing,
		const UClass* Type
	) const;

public:
	virtual TSharedPtr<SWidget> MakeContextMenu(const UObject* Parent) const;

private:
	// draw object bounds visualization
	void DrawDebugTarget(FPrimitiveDrawInterface* PDI, const FVector& Location, const FLinearColor& Color) const;

	HCrvHitProxy* LastHitProxy = nullptr;

	static FVector GetActorOrigin(const AActor* Actor);

protected:
	inline static FCrvRefCache Cache;
	inline static bool bMultiple = false;
};
