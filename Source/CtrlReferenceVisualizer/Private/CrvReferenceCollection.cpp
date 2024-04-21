#include "CrvReferenceCollection.h"

#include "CtrlReferenceVisualizer.h"
#include "ReferenceVisualizerComponent.h"
#include "CrvSettings.h"
#include "Selection.h"

#include "Styling/SlateIconFinder.h"

#include "UObject/ReferenceChainSearch.h"

// FReferenceChain has a IsExternal method but it does not appear to be exposed, causes linker error. Copied here.
bool IsExternal(FReferenceChainSearch::FReferenceChain* Chain)
{
	if (Chain == nullptr) { return false; }
	
	if (Chain->Num() > 1)
	{
		// Reference is external if the root (the last node) is not in the first node (target)
		return !Chain->GetRootNode()->ObjectInfo->IsIn(Chain->GetNode(0)->ObjectInfo);
	}
	else
	{
		return false;
	}
}

FString GetObjectFlagsString(const UObject* InObject)
{
	if (!InObject) { return FString(""); }
	TArray<FString> Flags;
	if (!IsValid(InObject))
	{
		Flags.Add(TEXT("Invalid"));
	}
	if (InObject->IsRooted())
	{
		Flags.Add(TEXT("Root"));
	}

	if (InObject->HasAnyFlags(RF_ClassDefaultObject))
	{
		Flags.Add(TEXT("CDO"));
	}
	
	if (InObject->HasAnyFlags(RF_Transient))
	{
		Flags.Add(TEXT("Transient"));
	}
	if (InObject->HasAnyFlags(RF_Public))
	{
		Flags.Add(TEXT("Public"));
	}
	if (InObject->HasAnyFlags(RF_Standalone))
	{
		Flags.Add(TEXT("Standalone"));
	}
	if (InObject->HasAnyFlags(RF_NeedLoad))
	{
		Flags.Add(TEXT("NeedLoad"));
	}
	if (InObject->HasAnyFlags(RF_NeedPostLoad))
	{
		Flags.Add(TEXT("NeedPostLoad"));
	}
	if (InObject->HasAnyFlags(RF_NeedPostLoadSubobjects))
	{
		Flags.Add(TEXT("NeedPostLoadSubobjects"));
	}
	if (InObject->HasAnyFlags(RF_WasLoaded))
	{
		Flags.Add(TEXT("WasLoaded"));
	}

	CA_SUPPRESS(6011)
	if (InObject->IsNative())
	{
		Flags.Add(TEXT("Native"));
	}

	if (InObject->HasAnyInternalFlags(EInternalObjectFlags::Garbage))
	{
		Flags.Add(TEXT("Garbage"));
	}

	if (InObject->HasAnyInternalFlags(EInternalObjectFlags::Async))
	{
		Flags.Add(TEXT("Async"));
	}

	if (InObject->HasAnyInternalFlags(EInternalObjectFlags::AsyncLoading))
	{
		Flags.Add( TEXT("AsyncLoading"));
	}

	return FString::Printf(TEXT("%s"), *FString::Join(Flags, TEXT(", ")));
}

FCrvMenuItem FCrvRefCollection::MakeMenuEntry(const UObject* Object)
{
	if (!Object) { return {}; }
	auto const FlagsString = GetObjectFlagsString(Object);
	auto const PackageShortName = FPackageName::GetShortName(Object->GetPackage()->GetName());
	auto const Tooltip = FText::FromString(FString::Printf(TEXT("Name: %s\nPkgName: %s\nFull Name: %s\nPath: %s\nFlags: %s"), *GetNameSafe(Object),  *PackageShortName, *GetFullNameSafe(Object), *GetPathNameSafe(Object), *FlagsString));
	if (const auto Actor = Cast<AActor>(Object))
	{
		auto const Label = FText::FromString(FString::Printf(TEXT("%s"),  *Actor->GetHumanReadableName()));
		auto const ActorTooltip = FText::FromString(FString::Printf(TEXT("Guid: %s\n%ls"),*Actor->GetActorInstanceGuid().ToString(), *Tooltip.ToString()));
		return {
			Actor->GetFName(),
			Label,
			ActorTooltip,
			FSlateIconFinder::FindIconForClass(Actor->GetClass()),
			FUIAction(
				FExecuteAction::CreateLambda(
					[Actor]()
					{
						GEditor->SelectNone(false, true);
						GEditor->SelectActor(const_cast<AActor*>(Actor), true, true);
						if (GetDefault<UCrvSettings>()->bMoveViewportCameraToReference)
						{
							GEditor->MoveViewportCamerasToActor(*const_cast<AActor*>(Actor), false);
						}
					}
				)
			)
		};
	}
	else if (const auto Component = Cast<UActorComponent>(Object))
	{
		
		auto const Label = FText::FromString(FString::Printf(TEXT("%s"),  *Component->GetReadableName()));
		return {
			Component->GetFName(),
			Label,
			Tooltip,
			FSlateIconFinder::FindIconForClass(Component->GetClass()),
			FUIAction(
				FExecuteAction::CreateLambda(
					[Component]()
					{
						GEditor->SelectNone(false, true);
						GEditor->SelectComponent(const_cast<UActorComponent*>(Component), true, true);
						if (GetDefault<UCrvSettings>()->bMoveViewportCameraToReference)
						{
							if (const auto SceneComponent = Cast<USceneComponent>(Component))
							{
								GEditor->MoveViewportCamerasToComponent(SceneComponent, false);
							}
							else
							{
								GEditor->MoveViewportCamerasToActor(*Component->GetOwner(), false);
							}
						}
					}
				)
			)
		};
	}

	return {
		Object->GetFName(),
		FText::FromString(Object->GetName()),
		Tooltip,
		FSlateIconFinder::FindIconForClass(Object->GetClass()),
		FUIAction()
	};
}

bool IsObjectProperty(const FProperty* InProperty)
{
	return CastField<FObjectPropertyBase>(InProperty) != nullptr;
}

static void RecursiveSearch(
	UObject* Target,
	TSet<UObject*>& Visited,
	const UCrvSettings* Settings,
	TFunction<void(UObject*, TSet<UObject*>&)> InSearchFunction,
	uint32 Depth = 0
)
{
	if (!IsValid(Target)) { return; }
	if (Visited.Contains(Target)) { return; }
	auto NumVisited = Visited.Num();
	Visited.Add(Target);
	auto bDebugEnabled = Settings->bDebugEnabled;
	auto Indent = FString::ChrN(Depth * 1, TEXT('|'));
	auto Name = FString::Printf(TEXT("%s (%s)"), *GetNameSafe(Target), Target->GetClass() ? *Target->GetClass()->GetName() : TEXT("null"));
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("%s RecursiveSearch for %s [Num %d] >>"), *Indent,  *Name, Visited.Num());

	InSearchFunction(Target, Visited);
	
	// iterate over object properties
	if (Settings->bWalkObjectProperties && Target->GetClass())
	{
		for (TFieldIterator<FObjectPropertyBase> PropIt(Target->GetClass(), EFieldIterationFlags::IncludeSuper); PropIt; ++PropIt)
		{
			if (const auto Prop = CastField<FSoftObjectProperty>(*PropIt))
			{
				const auto PropValue = Prop->GetPropertyValue_InContainer(Target);
				if (PropValue.IsNull()) { continue; }
				auto const Value = PropValue.Get();
				if (!Value) { continue; }

				InSearchFunction(Value, Visited);
				continue;
			}
			if (auto const ObjectProperty = CastField<FWeakObjectProperty>(*PropIt))
			{
				const auto PropValue = ObjectProperty->GetPropertyValue_InContainer(Target);
				if (!PropValue.IsValid()) { continue; }
				InSearchFunction(PropValue.Get(), Visited);
				continue;
			}
			if (auto const ObjectProperty = CastField<FObjectProperty>(*PropIt))
			{
				const auto PropValue = ObjectProperty->GetPropertyValue_InContainer(Target);
				if (!PropValue || !PropValue.Get()) { continue; }
				if (!IsValid(PropValue.Get())) { continue; }
				InSearchFunction(PropValue.Get(), Visited);
				continue;
			}
			if (auto const ObjectProperty = CastField<FLazyObjectProperty>(*PropIt))
			{
				const auto PropValue = ObjectProperty->GetPropertyValue_InContainer(Target);
				if (!PropValue.IsValid()) { continue; }
				InSearchFunction(PropValue.Get(), Visited);
				continue;
			}
		}
	}

	if (!Settings->bIsRecursive)
	{
		return;
	}
	
	if (const auto Actor = Cast<AActor>(Target))
	{
		if (Settings->bWalkComponents)
		{
			for (const auto Comp : Actor->GetComponents())
			{
				if (Comp->IsA<UReferenceVisualizerComponent>()) { continue; } // ignore our own component
				InSearchFunction(Comp, Visited);
			}
		}

		if (Settings->bWalkChildActors)
		{
			for (auto Child : Actor->Children)
			{
				RecursiveSearch(Child, Visited, Settings, InSearchFunction, Depth + 1);
			}
		}
	}
	if (const auto Scene = Cast<USceneComponent>(Target))
	{
		if (Settings->bWalkSceneChildComponents)
		{
			TArray<USceneComponent*> ChildComponents;
			Scene->GetChildrenComponents(true, ChildComponents);
			for (const auto Comp : ChildComponents)
			{
				InSearchFunction(Comp, Visited);
			}
		}
	}
	
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("RecursiveSearch for %s [Num: %d (+%d)] <<"), *Name, Visited.Num(), Visited.Num() - NumVisited);
	if (bDebugEnabled)
	{
		for (auto const& Obj : Visited)
		{
			if (Obj->IsA<AActor>() || !Obj->IsInOuter(Target))
			{
				UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("\t\tVisited: %s (%s): %s"), *GetNameSafe(Obj), *Obj->GetClass()->GetName(), *GetPathNameSafe(Obj));
			}
		}
	}
}

bool FCrvRefCollection::CanDisplayReference(const UObject* Target, const UObject* Object)
{
	if (!IsValid(Object)) { return false; }
	if (!IsValid(Target)) { return false; }
	if (!Object->IsA<AActor>() && Object->IsInOuter(Target)) { return false; }
	const auto CrvSettings = GetDefault<UCrvSettings>();
	if (CrvSettings->bIgnoreTransient && Object->HasAnyFlags(RF_Transient)) { return false; }
	// if (CrvSettings->bIgnoreArchetype && Object->HasAnyFlags(RF_ArchetypeObject)) { return false; }
	if (CrvSettings->TargetSettings.IgnoreReferencesToClasses.Contains(Object->GetClass())) { return false; }
	if (Object->IsA<UReferenceVisualizerComponent>()) { return false; }
	if (Object->IsA<AActor>()) { return true; }

	if (Object->IsA<UActorComponent>())
	{
		return CrvSettings->bShowComponents;
	}

	return CrvSettings->bShowObjects;
}

TSet<UObject*> FCrvRefCollection::GetSelectionSet()
{
	TArray<UObject*> SelectedActors;
	TArray<UObject*> SelectedComponents;
	GEditor->GetSelectedActors()->GetSelectedObjects(SelectedActors);
	GEditor->GetSelectedComponents()->GetSelectedObjects(SelectedComponents);
	TSet<UObject*> Selected;
	Selected.Append(SelectedActors);
	Selected.Append(SelectedComponents);
	return Selected;
}

static auto GetCanDisplayReference(UObject* Target)
{
	return [Target](const UObject* Object)
	{
		return FCrvRefCollection::CanDisplayReference(Target, Object);
	};
}

static void FilterRefs(UObject* Target, TSet<UObject*>& Visited)
{
	Visited.Remove(Target);
	const auto Current = Visited;
	Visited.Reset(); // keeps allocations
	Visited.Append(
		Current.Array().FilterByPredicate(GetCanDisplayReference(Target))
	);
	Visited.Compact();
};

TSet<UObject*> FCrvRefCollection::FindOutRefs(const UObject* Target)
{
	return FindOutRefs(const_cast<UObject*>(Target));
}

TSet<UObject*> FCrvRefCollection::FindOutRefs(UObject* Target)
{
	TSet<UObject*> Visited;
	FindOutRefs(Target, Visited);
	return Visited;
}

void FCrvRefCollection::FindOutRefs(const UObject* Target, TSet<UObject*>& Visited)
{
	FindOutRefs(const_cast<UObject*>(Target), Visited);
}

void FCrvRefCollection::FindRefs(UObject* Target, TSet<UObject*>& Visited, bool bIsOutgoing)
{
	if (bIsOutgoing)
	{
		FindOutRefs(Target, Visited);
	}
	else
	{
		FindInRefs(Target, Visited);
	}
}

TSet<UObject*> FCrvRefCollection::FindRefs(UObject* Target, bool bIsOutgoing)
{
	TSet<UObject*> Visited;
	FindRefs(Target, Visited, bIsOutgoing);
	return Visited;
}

void FCrvRefCollection::FindOutRefs(UObject* Target, TSet<UObject*>& Visited)
{
	if (!IsValid(Target)) { return; }
	auto* const CrvSettings = GetDefault<UCrvSettings>();
	auto const bDebugEnabled = CrvSettings->bDebugEnabled;
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("Finding FindOutRefs for %s [Num %d] >>"), *GetNameSafe(Target), Visited.Num());
	RecursiveSearch(
		Target,
		Visited,
		CrvSettings,
		[CrvSettings](UObject* Object, TSet<UObject*>& InVisited)
		{
			if (!Object) { return; }
			auto const Initial = InVisited.Num();
			auto const bDebugEnabled = CrvSettings->bDebugEnabled;
			if (CrvSettings->TargetSettings.IgnoreReferencesToClasses.Contains(Object->GetClass())) { return; }
			TArray<UObject*> OutReferences;
			FReferenceFinder RefFinder(
				OutReferences,
				nullptr,
				false,
				CrvSettings->bIgnoreArchetype,
				false,
				CrvSettings->bIgnoreTransient
			);

			RefFinder.FindReferences(Object);
			InVisited.Append(OutReferences);
			
			// find inner references
			TArray<UObject*> OutReferences2;
			FReferenceFinder RefFinder2(
				OutReferences2,
				Object,
				false,
				CrvSettings->bIgnoreArchetype,
				true,
				CrvSettings->bIgnoreTransient
			);
			
			RefFinder2.FindReferences(Object);
			auto const NumVisited = InVisited.Num();
			InVisited.Append(OutReferences2);
			auto const NumVisitedAfter = InVisited.Num();
			UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("Found %d Outgoing References for %s [Num: %d (First Pass: +%d, Second Pass: +%d)]"), OutReferences.Num(), *GetNameSafe(Object), NumVisited, NumVisited - Initial, NumVisitedAfter - NumVisited);
		}
	);
	auto const BeforeFiltering = Visited.Num();
	FilterRefs(Target, Visited);
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("Finding FindOutRefs for %s << [BeforeFiltering: %d Num: %d]"), *GetNameSafe(Target), BeforeFiltering, Visited.Num());
}

TArray<UObject*> FCrvRefCollection::GetOutRefs(const UObject* Target)
{
	return FindOutRefs(Target).Array();
}

TArray<UObject*> FCrvRefCollection::GetOutRefs(UObject* Target)
{
	return FindOutRefs(Target).Array();
}

TArray<UObject*> FCrvRefCollection::GetInRefs(const UObject* Target)
{
	return FindInRefs(Target).Array();
}

TArray<UObject*> FCrvRefCollection::GetInRefs(UObject* Target)
{
	return FindInRefs(Target).Array();
}

TSet<UObject*> FCrvRefCollection::FindInRefs(const UObject* Target)
{
	return FindInRefs(const_cast<UObject*>(Target));
}

TSet<UObject*> FCrvRefCollection::FindInRefs(UObject* Target)
{
	TSet<UObject*> Visited;
	FindInRefs(Target, Visited);
	return Visited;
}

void FCrvRefCollection::FindInRefs(const UObject* Target, TSet<UObject*>& Visited)
{
	FindInRefs(const_cast<UObject*>(Target), Visited);
}

void FCrvRefCollection::FindInRefs(UObject* Target, TSet<UObject*>& Visited)
{
	if (!IsValid(Target)) { return; }
	const auto CrvSettings = GetMutableDefault<UCrvSettings>();
	const auto SearchSettings = DuplicateObject(CrvSettings, GetTransientPackage());
	// disable recursion for incoming references, too expensive
	SearchSettings->bIsRecursive = false;
	TSet<UObject*> ChainRoots;
	TSet<UObject*> ChainVisited = Visited;

	TSet<FReferenceChainSearch::FReferenceChain*> Chains;
	RecursiveSearch(
		Target,
		ChainVisited,
		SearchSettings,
		[SearchSettings, &ChainRoots, Target](UObject* Object, TSet<UObject*>& InVisited)
		{
			if (!Object) { return; }
			if (Object != Target && !CanDisplayReference(Target, Object)) { return; }
			const FReferenceChainSearch RefChainSearch(
				Object,
				EReferenceChainSearchMode::ExternalOnly | EReferenceChainSearchMode::Direct
			);
			for (FReferenceChainSearch::FReferenceChain* Chain : RefChainSearch.GetReferenceChains())
			{
				if (!IsExternal(Chain)) { continue; }
				auto Root = Chain->GetRootNode()->ObjectInfo->TryResolveObject();
				if (!Root) { continue; }
				if (InVisited.Contains(Root)) { continue; }
				
				ChainRoots.Add(Root);
				
				TArray<UObject> ChainObjects;

				FString ChainString;
				for (int32 i = 0; i < Chain->Num(); i++)
				{
					auto const Node = Chain->GetNode(i);
					auto ChainObj = Node->ObjectInfo->TryResolveObject();
					if (!ChainObj) { continue; }
					InVisited.Add(ChainObj);
					ChainString += FString::Printf(TEXT("%s (%s) -> "), *GetNameSafe(Object), ChainObj->GetClass() ? *ChainObj->GetClass()->GetName() : TEXT("null"));
				}
				UE_LOG(LogCrv, Log, TEXT("Found Incoming Reference %s <- %s"), *GetNameSafe(Object), *ChainString);
				InVisited.Add(Root);
			}
		}
	);
	Visited.Append(ChainRoots);

	FilterRefs(Target, Visited);
}
