#include "CrvDebugVisualizer.h"

#include "CtrlReferenceVisualizer.h"
#include "CrvReferenceCollection.h"
#include "CrvSettings.h"
#include "SceneManagement.h"
#include "SEditorViewport.h"
#include "Selection.h"
#include "Editor/UnrealEdEngine.h"

#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"

#include "Layout/WidgetPath.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

void FCrvDebugVisualizerCache::Invalidate()
{
	if (bCached || Outgoing.Num() > 0 || Incoming.Num() > 0)
	{
		auto const bDebugEnabled = GetDefault<UCrvSettings>()->bDebugEnabled;
		UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("Invalidating cache..."));
	}
	Target = nullptr;
	Component = nullptr;
	Outgoing.Reset();
	Incoming.Reset();
	bCached = false;
	bHadValidItems = false;
}

TSet<UObject*> FCrvDebugVisualizerCache::GetValidCached(bool const bIsOutgoing)
{
	if (!bCached) { return {}; }
	auto Cached = bIsOutgoing ? Outgoing : Incoming;
	TSet<UObject*> Visited;
	Visited.Reserve(Cached.Num());
	bool bInvalidated = false;
	for (const auto Ref : Cached)
	{
		if (!Ref.IsValid())
		{
			bInvalidated = true;
			continue;
		}
		Visited.Add(Ref.Get());
	}
	if (bInvalidated)
	{
		Invalidate();
	}
	return Visited;
}

void FCrvDebugVisualizerCache::FillReferences(UObject* InTarget, const UActorComponent* InComponent, bool const bIsOutgoing)
{
	if (!IsValid(InTarget) || !IsValid(InComponent))
	{
		Invalidate();
		return;
	}
	auto const bDebugEnabled = GetDefault<UCrvSettings>()->bDebugEnabled;
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("FillReferences cache... Target %s Component %s | %s"), *GetNameSafe(InTarget), *GetNameSafe(InComponent), bIsOutgoing ? TEXT("Outgoing") : TEXT("Incoming"));
	TSet<UObject*> Visited;
	FCrvRefCollection::FindRefs(Target.Get(), Visited, bIsOutgoing);
	auto&& CachedItems = bIsOutgoing ? Outgoing : Incoming;
	CachedItems.Empty(Visited.Num());
	if (Target == Component)
	{
		Visited.Add(Component->GetOwner());
	}
	for (const auto DstRef : Visited)
	{
		CachedItems.Add(DstRef);
	}
	CachedItems.Compact();

	if (!bHadValidItems)
	{
		UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("FillReferences Cache has %d valid items."), CachedItems.Num());
		bHadValidItems = CachedItems.Num() > 0;
	}
}

void FCrvDebugVisualizerCache::FillCache(UObject* InTarget, const UActorComponent* InComponent, bool const bMultiple)
{
	if (Target != InTarget || Component != InComponent)
	{
		Invalidate();
		Target = InTarget;
		Component = InComponent;
	}

	if (!Target.IsValid() || !Component.IsValid())
	{
		Invalidate();
		return;
	}

	if (bCached) { return; }

	const auto Config = GetDefault<UCrvSettings>();
	if (Config->bShowOutgoingReferences)
	{
		FillReferences(Target.Get(), Component.Get(), true);
	}
	else
	{
		Outgoing.Empty();
	}

	if (Config->bShowIncomingReferences && !bMultiple)
	{
		FillReferences(Target.Get(), Component.Get(), false);
	}
	else
	{
		Incoming.Empty();
	}

	bCached = true;
}

void FCrvDebugVisualizer::OnRegister()
{
	FComponentVisualizer::OnRegister();

	const auto Config = GetMutableDefault<UCrvSettings>();
	Config->OnModified.RemoveAll(this);
	Config->OnModified.AddLambda(
		[this](UObject*, FProperty*)
		{
			Cache.Invalidate();
		}
	);
}

void FCrvDebugVisualizer::DrawVisualization(
	const UActorComponent* Component,
	const FSceneView* View,
	FPrimitiveDrawInterface* PDI
)
{
	const auto Config = GetDefault<UCrvSettings>();
	if (!Config->bIsEnabled)
	{
		InvalidateCache();
		return;
	}

	if (!IsValid(Component))
	{
		const bool bHadValidItems = Cache.bHadValidItems;
		InvalidateCache();
		if (bHadValidItems)
		{
			UE_LOG(LogCrv, Log, TEXT("Component reference invalid, probably changed. Refreshing selection to fix... %s"), *FString(__FUNCTION__));
			FCrvModule::Get().Refresh(true); // only refresh if we had valid items cached before and now we don't
		}
		return;
	}

	const auto Owner = Component->GetOwner();
	if (!ensure(IsValid(Owner)))
	{

		InvalidateCache();
		return;
	}

	TArray<UActorComponent*> SelectedComponents;
	ensure(GEditor);
	GEditor->GetSelectedComponents()->GetSelectedObjects<UActorComponent>(SelectedComponents);
	TArray<AActor*> SelectedActors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);
	bMultiple = SelectedActors.Num() > 1 || SelectedComponents.Num() > 1;
	if (SelectedComponents.Num() == 0 || Config->bAlwaysIncludeReferencesFromParent)
	{
		// draw references for the actor
		DrawVisualizationForComponent(Owner, Component, PDI);
	}
	else
	{
		// draw references for selected component(s)
		for (const auto SelectedComponent : SelectedComponents)
		{
			if (SelectedComponent->GetOwner() != Owner) { continue; }
			DrawVisualizationForComponent(SelectedComponent, SelectedComponent, PDI);
		}
	}
}

TSharedPtr<SWidget> FCrvDebugVisualizer::GenerateContextMenu() const
{
	return FComponentVisualizer::GenerateContextMenu();
}

void FCrvDebugVisualizer::DrawVisualizationForComponent(
	UObject* Target,
	const UActorComponent* Component,
	FPrimitiveDrawInterface* PDI
) const
{
	Cache.FillCache(Target, Component, bMultiple);
	const auto SourceLocation = GetComponentLocation(Component);
	const auto Config = GetDefault<UCrvSettings>();
	DrawDebugTarget(PDI, SourceLocation, Config->Style.CurrentCircleColor);
	DrawLinks(PDI, Component, true);
	DrawLinks(PDI, Component, false);
}

FVector FCrvDebugVisualizer::GetComponentLocation(const UActorComponent* Component)
{
	if (!IsValid(Component))
	{
		UE_LOG(LogCrv, Warning, TEXT("Invalid component passed to %s"), *FString(__FUNCTION__));
		return FVector::ZeroVector;
	}
	const auto Owner = Component->GetOwner();
	// use component location if it is a scene component
	const auto SceneComponent = Cast<USceneComponent>(Component);
	return SceneComponent ? SceneComponent->GetComponentLocation() : GetActorOrigin(Owner);
}

void FCrvDebugVisualizer::DrawLinks(
	FPrimitiveDrawInterface* PDI,
	const UActorComponent* Component,
	bool const bIsOutgoing
) const
{
	const auto Owner = Component->GetOwner();
	const FVector SourceLocation = GetComponentLocation(Component);
	const auto Config = GetDefault<UCrvSettings>();
	if (bIsOutgoing && !Config->bShowOutgoingReferences || !bIsOutgoing && !Config->bShowIncomingReferences)
	{
		return;
	}

	TSet<UObject*> ReferencedObjects = Cache.GetValidCached(bIsOutgoing);
	TSet<UObject*> OtherReferencedObjects = Cache.GetValidCached(!bIsOutgoing);
	// draw links to referenced objects
	for (const auto DstRef : ReferencedObjects)
	{
		// handle actor references
		if (const auto DstActor = Cast<AActor>(DstRef))
		{
			if (DstActor == Owner) { continue; }
			auto DstLocation = GetActorOrigin(DstActor);
			const auto HitProxy = new HCrvHitProxy(Component, DstActor);
			if (bIsOutgoing || bMultiple)
			{
				DrawDebugTarget(PDI, DstLocation, Config->Style.LinkedCircleColor);
				PDI->SetHitProxy(HitProxy);
				DrawDebugLink(PDI, SourceLocation, DstLocation, Config->Style.LineColor);
				PDI->SetHitProxy(nullptr);
			}
			else if (!OtherReferencedObjects.Contains(Component))
			{
				DrawDebugTarget(PDI, SourceLocation, Config->Style.CurrentCircleColor);
				PDI->SetHitProxy(HitProxy);
				DrawDebugLink(PDI, DstLocation, SourceLocation, Config->Style.LineColorIncoming);
				PDI->SetHitProxy(nullptr);
			}
			PDI->SetHitProxy(nullptr);
			continue;
		}

		// handle component references
		if (const auto DstActorComponent = Cast<UActorComponent>(DstRef))
		{
			if (DstActorComponent == Component) { continue; }
			const auto DstOwner = DstActorComponent->GetOwner();
			if (!IsValid(DstOwner)) { continue; }
			if (DstOwner == Owner) { continue; }

			const FVector DestLocation = GetComponentLocation(DstActorComponent);
			const auto DestActorHitProxy = new HCrvHitProxy(Component, DstActorComponent);

			if (bIsOutgoing || bMultiple)
			{
				DrawDebugTarget(PDI, DestLocation, Config->Style.LinkedCircleColor);
				PDI->SetHitProxy(DestActorHitProxy);
				DrawDebugLink(PDI, SourceLocation, DestLocation, Config->Style.LineColor);
				PDI->SetHitProxy(nullptr);
			}
			else
			{
				DrawDebugTarget(PDI, SourceLocation, Config->Style.CurrentCircleColor);
				PDI->SetHitProxy(DestActorHitProxy);
				DrawDebugLink(PDI, SourceLocation, DestLocation, Config->Style.LineColorIncoming);
				PDI->SetHitProxy(nullptr);
			}
			PDI->SetHitProxy(nullptr);
		}
	}
}

void FCrvDebugVisualizer::DrawDebugLink(
	FPrimitiveDrawInterface* PDI,
	const FVector& SrcOrigin,
	const FVector& DstOrigin,
	const FLinearColor& Color
) const
{
	const FVector Direction = (DstOrigin - SrcOrigin).GetSafeNormal();
	const auto Config = GetDefault<UCrvSettings>();
	float Distance = FVector::Distance(SrcOrigin, DstOrigin) - (Config->Style.CircleRadius * 2);
	Distance = FMath::Max(1.f, Distance); // clamp distance to be at least 1
	const auto SpacedSrcOrigin = SrcOrigin + Direction * Config->Style.CircleRadius;
	const auto SpacedDstOrigin = SpacedSrcOrigin + Direction * Distance;

	if (Config->Style.LineStyle == ECrvLineStyle::Dash)
	{
		DrawDashedLine(
			PDI,
			SpacedSrcOrigin,
			SpacedDstOrigin,
			Color,
			Config->Style.ArrowSize / 2.f,
			SDPG_Foreground
		);
	}
	else if (Config->Style.LineStyle == ECrvLineStyle::Arrow)
	{
		FVector YAxis, ZAxis;
		Direction.FindBestAxisVectors(YAxis, ZAxis);

		const FMatrix ArrowTM(Direction, YAxis, ZAxis, SpacedSrcOrigin);
		DrawDirectionalArrow(
			PDI,
			ArrowTM,
			Color,
			Distance,
			Config->Style.ArrowSize,
			SDPG_Foreground,
			Config->Style.LineThickness
		);
	}
}

TSharedPtr<SWidget> FCrvDebugVisualizer::MakeContextMenu() const
{
	FMenuBuilder MenuBuilder(true, nullptr);
	if (LastHitProxy && LastHitProxy->IsValid())
	{
		if (const auto Object = LastHitProxy->TargetObject.Get())
		{
			MenuBuilder.BeginSection(
				NAME_None,
				LOCTEXT("HoveredReference", "Hovered Reference")
			);
			const auto MenuEntry = FCrvRefCollection::MakeMenuEntry(Object);
			MenuBuilder.AddMenuEntry(
				MenuEntry.Label,
				MenuEntry.ToolTip,
				MenuEntry.Icon,
				MenuEntry.Action
			);
			MenuBuilder.EndSection();
		}
	}
	return MenuBuilder.MakeWidget();
}

void FCrvDebugVisualizer::DrawDebugTarget(
	FPrimitiveDrawInterface* PDI,
	const FVector& Location,
	const FLinearColor& Color
) const
{
	const auto Config = GetDefault<UCrvSettings>();
	if (!Config->Style.bDrawTargetCircles) { return; }
	DrawCircle(
		PDI,
		Location,
		FVector::ForwardVector,
		FVector::RightVector,
		Color,
		Config->Style.CircleRadius,
		Config->Style.CircleResolution,
		SDPG_Foreground,
		Config->Style.LineThickness
	);
}

FVector FCrvDebugVisualizer::GetActorOrigin(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogCrv, Warning, TEXT("Invalid actor passed to %s"), *FString(__FUNCTION__));
		return FVector::ZeroVector;
	}
	const auto& Config = *GetDefault<UCrvSettings>();
	if (Config.bUseActorBounds)
	{
		const auto BoundingBox = Actor->GetComponentsBoundingBox(Config.bIncludeNonCollidingBounds, Config.bIncludeChildActorsInBounds);
		if (BoundingBox.IsValid && !BoundingBox.ContainsNaN())
		{
			FVector Center;
			FVector Extents;
			BoundingBox.GetCenterAndExtents(Center, Extents);
			if (!Extents.IsNearlyZero())
			{
				return Center;
			}
		}
	}
	return Actor->GetActorLocation();
}

bool FCrvDebugVisualizer::VisProxyHandleClick(
	FEditorViewportClient* InViewportClient,
	HComponentVisProxy* VisProxy,
	const FViewportClick& Click
)
{
	FComponentVisualizer::VisProxyHandleClick(InViewportClient, VisProxy, Click);
	if (VisProxy && VisProxy->Component.IsValid())
	{
		LastHitProxy = static_cast<HCrvHitProxy*>(VisProxy);
	}
	else
	{
		LastHitProxy = nullptr;
	}
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		const TSharedPtr<SWidget> MenuWidget = MakeContextMenu();
		if (MenuWidget.IsValid())
		{
			const TSharedPtr<SEditorViewport> ViewportWidget = InViewportClient->GetEditorViewportWidget();
			if (ViewportWidget.IsValid())
			{
				FSlateApplication::Get().PushMenu(
					ViewportWidget.ToSharedRef(),
					FWidgetPath(),
					MenuWidget->AsShared(),
					FSlateApplication::Get().GetCursorPos(),
					FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
				);

				return true;
			}
		}
	}
	return false;
}

void FCrvDebugVisualizer::InvalidateCache()
{
	Cache.Invalidate();
}

#undef LOCTEXT_NAMESPACE
