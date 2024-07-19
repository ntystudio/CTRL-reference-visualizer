#include "ReferenceVisualizerComponent.h"

#include "CrvRefCache.h"
#include "CrvSettings.h"
#include "Selection.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
// Added in UE 5.2
#include "Materials/MaterialRenderProxy.h"
#endif

void UReferenceVisualizerEditorSubsystem::OnObjectModified(UObject* Object)
{
	if (Cache->WeakRootObjects.Contains(Object))
	{
		Cache->Invalidate(FString::Printf(TEXT("Object modified: %s"), *GetDebugName(Object)));
	}
}

void UReferenceVisualizerEditorSubsystem::OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	const FString PropertyChangeDescription = PropertyChangedEvent.GetMemberPropertyName().ToString();
	if (Cache->WeakRootObjects.Contains(Object))
	{
		Cache->Invalidate(FString::Printf(TEXT("Property modified: %s %s"), *GetDebugName(Object), *PropertyChangeDescription));
	}
}

void UReferenceVisualizerEditorSubsystem::OnSettingsModified(UObject* Object, FProperty* Property)
{
	UpdateCache();
}

UReferenceVisualizerEditorSubsystem::UReferenceVisualizerEditorSubsystem()
{
	Cache = CreateDefaultSubobject<UCrvRefCache>(TEXT("Cache"));
	MenuCache = CreateDefaultSubobject<UCrvRefCache>(TEXT("MenuCache"));
}

void UReferenceVisualizerEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UCrvSettings* Settings = GetMutableDefault<UCrvSettings>();
	Settings->OnModified.RemoveAll(this);
	SettingsModifiedHandle.Reset();
	SettingsModifiedHandle = Settings->OnModified.AddUObject(this, &UReferenceVisualizerEditorSubsystem::OnSettingsModified);
	// Settings->AddComponentClass(UReferenceVisualizerComponent::StaticClass());
	// add visualizer to actors when editor selection is changed
	USelection::SelectionChangedEvent.AddUObject(this, &UReferenceVisualizerEditorSubsystem::OnSelectionChanged);
	FCoreUObjectDelegates::OnObjectModified.AddUObject(this, &UReferenceVisualizerEditorSubsystem::OnObjectModified);
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UReferenceVisualizerEditorSubsystem::OnPropertyChanged);
}

void UReferenceVisualizerEditorSubsystem::OnSelectionChanged(UObject* SelectionObject)
{
	if (bIsRefreshingSelection)
	{
		return;
	}
	TGuardValue<bool> ReentrantGuard(bIsRefreshingSelection, true);
	UpdateCache();
}

void UReferenceVisualizerEditorSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UReferenceVisualizerEditorSubsystem::UpdateCache()
{
	Cache->ScheduleUpdate();
}

FDebugRenderSceneProxy* UReferenceVisualizerComponent::CreateDebugSceneProxy()
{
	FCtrlReferenceVisualizerSceneProxy* DebugProxy = new FCtrlReferenceVisualizerSceneProxy(this);
	Lines.Reset();
	CreateLines(GetOwner(), ECrvDirection::Outgoing);
	CreateLines(GetOwner(), ECrvDirection::Incoming);
	DebugProxy->DrawLines(Lines);
	Lines.Compact();
	return DebugProxy;
}

void UReferenceVisualizerComponent::CreateLines(
	const UObject* RootObject,
	const ECrvDirection Direction
)
{
	const auto Config = GetDefault<UCrvSettings>();
	if (Direction == ECrvDirection::Outgoing && !Config->bShowOutgoingReferences || Direction == ECrvDirection::Incoming && !Config->bShowIncomingReferences)
	{
		return;
	}

	FVector BaseOffset(0, 0, 10);
	TObjectPtr<UObject> RootObjectPtr = const_cast<UObject*>(RootObject);
	auto References = CrvEditorSubsystem->Cache->GetReferences(RootObjectPtr, Direction);
	if (!CrvEditorSubsystem->Cache->Contains(RootObjectPtr)) { return; }
	if (References.Num() == 0) { return; }
	// when multiple roots are selected, don't show incoming references that are also outgoing to same node
	if (Direction == ECrvDirection::Incoming && CrvEditorSubsystem->Cache->WeakRootObjects.Num() > 1)
	{
		References = TSet(References.Array().FilterByPredicate([this, RootObjectPtr](UObject* Ref)
		{
			const auto OutReferences = CrvEditorSubsystem->Cache->GetReferences(Ref, ECrvDirection::Outgoing);
			if (!OutReferences.Num()) { return true; }
			return !OutReferences.Contains(RootObjectPtr);
		}));
		if (References.Num() == 0) { return; }
	}
	const FVector SourceLocation = FCtrlReferenceVisualizerSceneProxy::GetObjectLocation(RootObject);
	// draw links to referenced objects
	Lines.Reserve(Lines.Num() + References.Num());
	UE_CLOG(FCrvModule::IsDebugEnabled(), LogCrv, Log, TEXT("References %s %s"), Direction == ECrvDirection::Outgoing ? TEXT(" Out ") : TEXT(" In "), *CtrlRefViz::GetDebugName(RootObject));
	for (const auto DstRef : References)
	{
		UE_CLOG(FCrvModule::IsDebugEnabled(), LogCrv, Log, TEXT("\t%s"), *CtrlRefViz::GetDebugName(DstRef));
		auto DstLocation = FCtrlReferenceVisualizerSceneProxy::GetObjectLocation(DstRef);
		auto Offset = Direction == ECrvDirection::Outgoing ? BaseOffset : -BaseOffset;
		auto Line = CreateLine(SourceLocation + Offset, DstLocation + Offset, Direction, DstRef->GetClass());
		Lines.Add(Line);
	}
}

FCrvLine UReferenceVisualizerComponent::CreateLine(const FVector& SrcOrigin, const FVector& DstOrigin, const ECrvDirection Direction, const UClass* Type)
{
	if (Direction == ECrvDirection::Incoming)
	{
		Swap(SrcOrigin, DstOrigin);
	}
	const FVector LineDirection = (DstOrigin - SrcOrigin).GetSafeNormal();
	const auto Config = GetDefault<UCrvSettings>();
	float Distance = FVector::Distance(SrcOrigin, DstOrigin) - (Config->Style.CircleRadius * 2);
	Distance = FMath::Max(1.f, Distance); // clamp distance to be at least 1
	auto SpacedSrcOrigin = SrcOrigin + LineDirection * Config->Style.CircleRadius;
	auto SpacedDstOrigin = SpacedSrcOrigin + LineDirection * Distance;
	auto LineStyle = Config->GetLineStyle(Direction);
	auto [LineType, LineColor, LineColorComponent, LineColorObject, LineThickness, ArrowSize, DepthPriority] = LineStyle;
	auto Color = LineColor;
	if (Type->IsChildOf(AActor::StaticClass()))
	{
		Color = LineColor;
	}
	else if (Type->IsChildOf(UActorComponent::StaticClass()))
	{
		Color = LineColorComponent;
	}
	else
	{
		Color = LineColorObject;
	}
	FCrvLine Line;
	Line.Start = SpacedSrcOrigin;
	Line.End = SpacedDstOrigin;
	Line.Style = LineStyle;
	Line.Color = Color.ToFColor(true);
	return Line;
}

void FCtrlReferenceVisualizerSceneProxy::GetDynamicMeshElementsForView(
	const FSceneView* View,
	const int32 ViewIndex,
	const FSceneViewFamily& ViewFamily,
	const uint32 VisibilityMap,
	FMeshElementCollector& Collector,
	FMaterialCache& DefaultMaterialCache,
	FMaterialCache& SolidMeshMaterialCache
) const
{
	FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

	// Draw Lines
	const int32 LinesNum = Lines.Num();
	PDI->AddReserveLines(SDPG_World, LinesNum, false, false);
	for (const FDebugLine& Line : Lines)
	{
		PDI->DrawLine(Line.Start, Line.End, Line.Color, DepthPriorityGroup, Line.Thickness, 0, Line.Thickness > 0);
	}

	// Draw Dashed Lines
	for (const FDashedLine& Dash : DashedLines)
	{
		DrawDashedLine(PDI, Dash.Start, Dash.End, Dash.Color, Dash.DashSize, DepthPriorityGroup);
	}

	// Draw Arrows
	const uint32 ArrowsNum = ArrowLines.Num();
	PDI->AddReserveLines(DepthPriorityGroup, 5 * ArrowsNum, false, false);
	for (const FArrowLine& ArrowLine : ArrowLines)
	{
		// draw a pretty arrow
		FVector Dir = ArrowLine.End - ArrowLine.Start;
		const float DirMag = Dir.Size();
		Dir /= DirMag;
		FVector YAxis, ZAxis;
		Dir.FindBestAxisVectors(YAxis, ZAxis);
		FMatrix ArrowTM(Dir, YAxis, ZAxis, ArrowLine.Start);
		DrawDirectionalArrow(
			PDI,
			ArrowTM,
			ArrowLine.Color,
			DirMag,
			8.f,
			DepthPriorityGroup,
			1.f
		);
	}

	// Draw Stars
	for (const FWireStar& Star : Stars)
	{
		Star.Draw(PDI);
	}

	// Draw Cylinders
	for (const FWireCylinder& Cylinder : Cylinders)
	{
		Cylinder.Draw(PDI, (Cylinder.DrawTypeOverride != EDrawType::Invalid) ? Cylinder.DrawTypeOverride : DrawType, DrawAlpha, DefaultMaterialCache, ViewIndex, Collector);
	}

	// Draw Boxes
	for (const FDebugBox& Box : Boxes)
	{
		Box.Draw(PDI, (Box.DrawTypeOverride != EDrawType::Invalid) ? Box.DrawTypeOverride : DrawType, DrawAlpha, DefaultMaterialCache, ViewIndex, Collector);
	}

	// Draw Cones
	TArray<FVector> Verts;
	for (const FCone& Cone : Cones)
	{
		Cone.Draw(PDI, (Cone.DrawTypeOverride != EDrawType::Invalid) ? Cone.DrawTypeOverride : DrawType, DrawAlpha, DefaultMaterialCache, ViewIndex, Collector, &Verts);
	}

	// Draw spheres
	for (const FSphere& Sphere : Spheres)
	{
		if (PointInView(Sphere.Location, View))
		{
			Sphere.Draw(PDI, (Sphere.DrawTypeOverride != EDrawType::Invalid) ? Sphere.DrawTypeOverride : DrawType, DrawAlpha, DefaultMaterialCache, ViewIndex, Collector);
		}
	}

	// Draw Capsules
	for (const FCapsule& Capsule : Capsules)
	{
		if (PointInView(Capsule.Base, View))
		{
			Capsule.Draw(PDI, (Capsule.DrawTypeOverride != EDrawType::Invalid) ? Capsule.DrawTypeOverride : DrawType, DrawAlpha, DefaultMaterialCache, ViewIndex, Collector);
		}
	}

	// Draw Meshes
	for (const FMesh& Mesh : Meshes)
	{
		FDynamicMeshBuilder MeshBuilder(View->GetFeatureLevel());
		MeshBuilder.AddVertices(Mesh.Vertices);
		MeshBuilder.AddTriangles(Mesh.Indices);

		// auto&& MeshMaterialCache = Mesh.Color.A == 255 ? SolidMeshMaterialCache : DefaultMaterialCache; 
		// MeshBuilder.GetMesh(FMatrix::Identity, MeshMaterialCache[Mesh.Color], SDPG_World, false, false, ViewIndex, Collector);
		// Parent caches these (only within this function) but let's assume that's not worth it. Will people really
		// have lots of meshes with a shared colour in this single context to make it worth it?
		const auto MatRenderProxy = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial->GetRenderProxy(), Mesh.Color);
		FDynamicMeshBuilderSettings Settings;
		Settings.bWireframe = true;
		Settings.bUseSelectionOutline = false;
		Settings.bUseWireframeSelectionColoring = true;
		MeshBuilder.GetMesh(FMatrix::Identity, MatRenderProxy, DepthPriorityGroup, Settings, nullptr, ViewIndex, Collector);
	}
}

void FCtrlReferenceVisualizerSceneProxy::DrawLines(TSet<FCrvLine> InLines)
{
	for (auto Line : InLines)
	{
		auto [LineType, LineColor, LineColorComponent, LineColorObject, LineThickness, ArrowSize, DepthPriority] = Line.Style;
		auto Direction = (Line.End - Line.Start).GetSafeNormal();
		auto Distance = FVector::Distance(Line.Start, Line.End);

		if (LineType == ECrvLineType::Dash)
		{
			DashedLines.Add(FDebugRenderSceneProxy::FDashedLine(Line.Start, Line.End, Line.Color.ToFColor(true), Distance / 20));
			auto ArrowStart = Line.Start + Direction * (Distance - ArrowSize * UE_GOLDEN_RATIO * 2);
			ArrowLines.Add(FDebugRenderSceneProxy::FArrowLine(ArrowStart, Line.End, Line.Color.ToFColor(true)));
		}
		else if (LineType == ECrvLineType::Arrow)
		{
			ArrowLines.Add(FDebugRenderSceneProxy::FArrowLine(Line.Start, Line.End, Line.Color.ToFColor(true)));
		}
	}
}

FVector FCtrlReferenceVisualizerSceneProxy::GetComponentLocation(const UActorComponent* Component)
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

FVector FCtrlReferenceVisualizerSceneProxy::GetActorOrigin(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogCrv, Warning, TEXT("Invalid actor passed to %s"), *FString(__FUNCTION__));
		return FVector::ZeroVector;
	}
	const auto Config = GetDefault<UCrvSettings>();
	if (Config->bUseActorBounds)
	{
		const auto BoundingBox = Actor->GetComponentsBoundingBox(Config->bIncludeNonCollidingBounds, Config->bIncludeChildActorsInBounds);
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

FCtrlReferenceVisualizerSceneProxy::FCtrlReferenceVisualizerSceneProxy(const UPrimitiveComponent* InComponent): FDebugRenderSceneProxy(InComponent) {}

SIZE_T FCtrlReferenceVisualizerSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

FPrimitiveViewRelevance FCtrlReferenceVisualizerSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bDynamicRelevance = true;
	Result.bShadowRelevance = false;
	Result.bStaticRelevance = true;
	Result.bEditorPrimitiveRelevance = true;
	Result.bEditorVisualizeLevelInstanceRelevance = true;
	return Result;
}

uint32 FCtrlReferenceVisualizerSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize();
}

FVector FCtrlReferenceVisualizerSceneProxy::GetObjectLocation(const UObject* Object)
{
	if (const auto Component = Cast<UActorComponent>(Object))
	{
		return GetComponentLocation(Component);
	}
	else if (const auto Actor = Cast<AActor>(Object))
	{
		return GetActorOrigin(Actor);
	}
	else if (const auto OwnerComponent = Object->GetTypedOuter<UActorComponent>())
	{
		return GetComponentLocation(OwnerComponent);
	}
	else if (const auto OwnerActor = Object->GetTypedOuter<AActor>())
	{
		return GetActorOrigin(OwnerActor);
	}

	return FVector::ZeroVector;
}

FBoxSphereBounds UReferenceVisualizerComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	auto SphereBounds = Super::CalcBounds(LocalToWorld);
	if (!GetOwner()) { return SphereBounds; }
	if (!CrvEditorSubsystem)
	{
		return SphereBounds;
	}
	const auto World = GetWorld();
	if (!World) { return SphereBounds; }
	if (!CrvEditorSubsystem->Cache->Contains(GetOwner()))
	{
		return SphereBounds;
	}

	FBoxSphereBounds::Builder DebugBoundsBuilder;
	DebugBoundsBuilder += SphereBounds;
	FVector Origin;
	FVector BoxExtent;
	GetOwner()->GetActorBounds(false, Origin, BoxExtent);
	FBox ActorBounds = FBox::BuildAABB(Origin, BoxExtent);
	DebugBoundsBuilder += ActorBounds;
	for (auto [Start, End, Color, Style] : Lines)
	{
		DebugBoundsBuilder += Start;
		DebugBoundsBuilder += End;
	}
	FBoxSphereBounds NewBounds = DebugBoundsBuilder; //.TransformBy(LocalToWorld);
	NewBounds = NewBounds.ExpandBy(100);
	return NewBounds;
}

void UReferenceVisualizerComponent::OnRegister()
{
	Super::OnRegister();
	CrvEditorSubsystem = GEditor->GetEditorSubsystem<UReferenceVisualizerEditorSubsystem>();
	CrvEditorSubsystem->Cache->OnCacheUpdated.AddUObject(this, &UReferenceVisualizerComponent::MarkRenderStateDirty);
	CrvEditorSubsystem->Cache->ScheduleUpdate();
}

void UReferenceVisualizerComponent::OnUnregister()
{
	Super::OnUnregister();
	if (CrvEditorSubsystem)
	{
		CrvEditorSubsystem->Cache->ScheduleUpdate();
	}
}

UReferenceVisualizerComponent::UReferenceVisualizerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bVisibleInReflectionCaptures = false;
	bVisibleInRayTracing = false;
	bVisibleInRealTimeSkyCaptures = false;
	bAutoActivate = true;
	AlwaysLoadOnClient = false;
	bIsEditorOnly = true;

	SetHiddenInGame(true);

#if WITH_EDITOR
	// Do not show bounding box for better visibility
	SetIsVisualizationComponent(true);

	// Disable use of bounds to focus to avoid de-zoom
	SetIgnoreBoundsForEditorFocus(true);

#endif
#if WITH_EDITORONLY_DATA
	// Show sprite for this component to visualize it when empty
	bVisualizeComponent = true;
#endif
}
