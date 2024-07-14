#include "CrvDebugVisualizer.h"

#include "CrvRefSearch.h"
#include "CrvSettings.h"
#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"
#include "SceneManagement.h"
#include "SEditorViewport.h"
#include "Selection.h"
#include "Editor/UnrealEdEngine.h"

#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"

#include "Layout/WidgetPath.h"

using namespace CRV;

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

void FCrvDebugVisualizer::OnRegister()
{
	FComponentVisualizer::OnRegister();
	UE_LOG(LogCrv, Log, TEXT("Registering %s"), *FString(__FUNCTION__));

	const auto Config = GetMutableDefault<UCrvSettings>();
	Config->OnModified.RemoveAll(this);
	Config->OnModified.AddLambda(
		[this](UObject*, const FProperty* Property)
		{
			Cache.Invalidate(FString::Printf(TEXT("Settings modified: %s"), *GetNameSafe(Property)));
		}
	);
}

void FCrvDebugVisualizer::DrawVisualization(
	const UActorComponent* RootComponent,
	const FSceneView* View,
	FPrimitiveDrawInterface* PDI
)
{
	if (RootComponent != LastRootComponent.Get())
	{
		LastRootComponent = RootComponent;
		UE_LOG(LogCrv, Log, TEXT("DrawVisualization %s %s %s"), *FString(__FUNCTION__), *CRV_LOG_LINE, *GetDebugName(RootComponent));
	}

	const auto Config = GetDefault<UCrvSettings>();
	if (!Config->bIsEnabled)
	{
		InvalidateCache(FString(TEXT("Disabled ")));
		return;
	}

	if (!IsValid(RootComponent))
	{
		const bool bHadValidItems = Cache.bHadValidItems;
		InvalidateCache(FString(TEXT("Invalid RootComponent")));
		if (bHadValidItems)
		{
			UE_LOG(LogCrv, Log, TEXT("RootComponent reference invalid, probably changed. Refreshing selection to fix... %s"), *FString(__FUNCTION__));
			FCrvModule::Get().QueueRefresh(true); // only refresh if we had valid items cached before and now we don't
		}
		return;
	}

	const auto Owner = RootComponent->GetOwner();
	if (!ensure(IsValid(Owner)))
	{

		InvalidateCache(FString(TEXT("Invalid Owner")));
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
		DrawVisualizationForRootObject(Owner, RootComponent, PDI);
	}
	else
	{
		// draw references for selected component(s)
		for (const auto SelectedComponent : SelectedComponents)
		{
			if (SelectedComponent->GetOwner() != Owner) { continue; }
			DrawVisualizationForRootObject(SelectedComponent, SelectedComponent, PDI);
		}
	}
}

TSharedPtr<SWidget> FCrvDebugVisualizer::GenerateContextMenu() const
{
	return FComponentVisualizer::GenerateContextMenu();
}

void FCrvDebugVisualizer::DrawVisualizationForRootObject(
	UObject* RootObject,
	const UActorComponent* RootComponent,
	FPrimitiveDrawInterface* PDI
) const
{
	Cache.FillCache(RootObject, bMultiple);
	const auto SourceLocation = GetComponentLocation(RootComponent);
	const auto Config = GetDefault<UCrvSettings>();
	DrawDebugTarget(PDI, SourceLocation, Config->Style.CurrentCircleColor);
	DrawLinks(PDI, RootComponent, RootObject, true);
	DrawLinks(PDI, RootComponent, RootObject, false);
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

FString GetDebugNames(TArray<UObject*> Objects, FString Separator = TEXT(", "))
{
	return FString::JoinBy(Objects, *Separator, [](const UObject* Obj)
	{
		return GetDebugName(Obj);
	});
}

void FCrvDebugVisualizer::OnHoverProxy(FCrvHitProxyRef HitProxy)
{
	// static FHitProxyId LastHoveredId;
	//
	// // don't log again if we're hovering over the same proxy
	// if (HitProxy.Id == LastHoveredId) { return; }
	//
	// LastHoveredId = HitProxy.Id;
	// if (!HitProxy.IsValid())
	// {
	// 	return;
	// }
	// for (const auto Chain : FCrvRefSearch::GetChainsBetween(HitProxy.RootObject.Get(), HitProxy.LeafObject.Get()))
	// {
	// 	if (!Chain)
	// 	{
	// 		UE_LOG(LogCrv, Log, TEXT("Hovered (No Chain) %s -> %s"), *GetDebugName(HitProxy.RootObject.Get()), *GetDebugName(HitProxy.LeafObject.Get()));
	// 		continue;
	// 	}
	// 	UE_LOG(LogCrv, Log, TEXT("Hovered Chain: %s"), *Chain->ToString());
	// }
}

void FCrvDebugVisualizer::DrawLinks(
	FPrimitiveDrawInterface* PDI,
	const UActorComponent* RootComponent,
	const UObject* RootObject,
	const bool bIsOutgoing
) const
{
	const auto Owner = RootComponent->GetOwner();
	const FVector SourceLocation = GetComponentLocation(RootComponent);
	const auto Config = GetDefault<UCrvSettings>();
	if (bIsOutgoing && !Config->bShowOutgoingReferences || !bIsOutgoing && !Config->bShowIncomingReferences)
	{
		return;
	}

	TSet<UObject*> ReferencedObjects = Cache.GetValidCached(bIsOutgoing);
	const TSet<UObject*> OtherReferencedObjects = Cache.GetValidCached(!bIsOutgoing);

	// draw links to referenced objects
	for (const auto DstRef : ReferencedObjects)
	{
		// handle actor references
		if (const auto DstActor = Cast<AActor>(DstRef))
		{
			if (DstActor == Owner) { continue; }
			auto DstLocation = GetActorOrigin(DstActor);
			const auto HitProxy = new HCrvHitProxy(RootComponent, RootObject, DstActor);
			FCrvHitProxyRef Ref = HitProxy;
			if (bIsOutgoing || bMultiple)
			{
				DrawDebugTarget(PDI, DstLocation, Config->Style.LinkedCircleColor);
				PDI->SetHitProxy(HitProxy);
				DrawDebugLink(PDI, SourceLocation, DstLocation, Config->Style.LineColor);
				PDI->SetHitProxy(nullptr);
			}
			else
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
			if (DstActorComponent == RootComponent) { continue; }
			const auto DstOwner = DstActorComponent->GetOwner();
			if (!IsValid(DstOwner)) { continue; }
			if (DstOwner == Owner) { continue; }

			const FVector DestLocation = GetComponentLocation(DstActorComponent);
			const auto HitProxy = new HCrvHitProxy(RootComponent, RootObject, DstActorComponent);

			FCrvHitProxyRef Ref = HitProxy;

			if (bIsOutgoing || bMultiple)
			{
				DrawDebugTarget(PDI, DestLocation, Config->Style.LinkedCircleColor);
				PDI->SetHitProxy(HitProxy);
				DrawDebugLink(PDI, SourceLocation, DestLocation, Config->Style.LineColor);
				PDI->SetHitProxy(nullptr);
			}
			else
			{
				DrawDebugTarget(PDI, SourceLocation, Config->Style.CurrentCircleColor);
				PDI->SetHitProxy(HitProxy);
				DrawDebugLink(PDI, DestLocation, SourceLocation, Config->Style.LineColorIncoming);
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

TSharedPtr<SWidget> FCrvDebugVisualizer::MakeContextMenu(const UObject* Parent) const
{
	if (!Parent) { return nullptr; }
	FMenuBuilder MenuBuilder(true, nullptr);
	if (LastHitProxy && LastHitProxy->IsValid())
	{
		if (const auto Object = LastHitProxy->LeafObject.Get())
		{
			MenuBuilder.BeginSection(
				NAME_None,
				LOCTEXT("HoveredReference", "Hovered Reference")
			);
			const auto MenuEntry = FCrvRefSearch::MakeMenuEntry(Parent, Object);
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
	UE_LOG(LogCrv, Log, TEXT("VisProxyHandleClick %s %s"), *Click.GetKey().ToString(), *UEnum::GetValueAsString(Click.GetEvent()))
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		const TSharedPtr<SWidget> MenuWidget = MakeContextMenu(LastHitProxy->LeafObject.Get());
		if (MenuWidget.IsValid())
		{
			const TSharedPtr<SEditorViewport> ViewportWidget = InViewportClient->GetEditorViewportWidget();
			if (ViewportWidget.IsValid())
			{
				FSlateApplication::Get().PushMenu(
					ViewportWidget.ToSharedRef(),
					FWidgetPath(),
					MenuWidget->AsShared(),
					FSlateApplication::Get().GetCursorPos(), // Click position doesn't account for multiple screens
					FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
				);

				return true;
			}
		}
	}
	return false;
}

void FCrvDebugVisualizer::InvalidateCache(const FString& Reason)
{
	Cache.Invalidate(Reason);
}

#undef LOCTEXT_NAMESPACE
