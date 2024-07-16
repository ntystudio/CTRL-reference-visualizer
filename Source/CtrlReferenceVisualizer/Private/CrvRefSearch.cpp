#include "CrvRefSearch.h"

#include "CrvSettings.h"
#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"
#include "ReferenceVisualizerComponent.h"
#include "Selection.h"

#include "Styling/SlateIconFinder.h"

#include "UObject/ReferenceChainSearch.h"
using namespace CRV;


namespace CRV::Search
{
	bool IsExternal(const FReferenceChainSearch::FReferenceChain* Chain)
	{
		if (Chain == nullptr)
		{
			return false;
		}

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


	TSet<UObject*> FindOwnedObjects(UObject* Target)
	{
		const auto CrvSettings = GetDefault<UCrvSettings>();
		TArray<UObject*> Owned;
		FReferenceFinder RefFinder(
			Owned,
			Target,
			false,
			CrvSettings->bIgnoreArchetype,
			CrvSettings->bIsRecursive,
			CrvSettings->bIgnoreTransient
		);

		RefFinder.FindReferences(Target);
		return TSet(Owned);
	}

	FString GetObjectFlagsString(const UObject* InObject)
	{
		if (!InObject)
		{
			return FString("");
		}
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
			Flags.Add(TEXT("AsyncLoading"));
		}

		return FString::Printf(TEXT("%s"), *FString::Join(Flags, TEXT(", ")));
	}
}

FString LexToString(const FReferenceChainSearch::FReferenceChain* Chain)
{
	FString Result;
	if (!Chain)
	{
		return Result;
	}
	if (const auto RootNode = Chain->GetRootNode())
	{
		Result += FString::Printf(
			TEXT("Chain Num %d External %s Root %s\n"),
			Chain->Num(),
			*::LexToString(Search::IsExternal(Chain)),
			*GetDebugName(RootNode->ObjectInfo->TryResolveObject())
		);
	}
	for (int32 i = 0; i < Chain->Num(); ++i)
	{
		const auto GraphNode = Chain->GetNode(i);
		auto Indent = FString::ChrN(i + 1, TEXT('\t'));
		if (const auto Object = GraphNode->ObjectInfo->TryResolveObject())
		{
			Result += FString::Printf(TEXT("%d %s %s"), i, *Indent, *GetDebugName(Object));
			if (const auto RefInfo = Chain->GetReferenceInfo(i))
			{
				Result += RefInfo->ToString();
			}
			Result += TEXT("\n");
		}
		for (const auto RefGraphNode : GraphNode->ReferencedByObjects)
		{
			Result += FString::Printf(TEXT("%s\tReferenced by %s\n"), *Indent, *GetDebugName(RefGraphNode->ObjectInfo->TryResolveObject()));
		}
	}
	return MoveTemp(Result);
}

FCrvMenuItem FCrvRefSearch::MakeMenuEntry(const UObject* Parent, const UObject* Object)
{
	if (!Object)
	{
		return {};
	}
	const auto FlagsString = Search::GetObjectFlagsString(Object);
	const auto PackageShortName = FPackageName::GetShortName(Object->GetPackage()->GetName());
	auto Tooltip = FString::Printf(TEXT("%s\nPath: %s\nPkgName: %s\nFlags: %s"), *GetDebugName(Object), *GetPathNameSafe(Object), *PackageShortName, *FlagsString);
	if (const auto OuterActor = Object->GetTypedOuter<AActor>())
	{
		if (OuterActor != Object)
		{
			Tooltip += FString::Printf(TEXT("\nOuterActor: %s"), *OuterActor->GetActorNameOrLabel());
		}
	}
	if (const auto OuterComponent = Object->GetTypedOuter<UActorComponent>())
	{
		if (OuterComponent != Object)
		{
			Tooltip += FString::Printf(TEXT("\nOuterComponent: %s"), *OuterComponent->GetReadableName());
		}
	}

	if (const auto Actor = Cast<AActor>(Object))
	{
		const auto Label = FText::FromString(FString::Printf(TEXT("%s"), *Actor->GetActorNameOrLabel()));
		const auto ActorTooltip = FText::FromString(FString::Printf(TEXT("%ls\nGuid: %s"), *Tooltip, *Actor->GetActorInstanceGuid().ToString()));
		return {
			Actor->GetFName(),
			Label,
			ActorTooltip,
			FSlateIconFinder::FindIconForClass(Actor->GetClass()),
			FUIAction(
				FExecuteAction::CreateWeakLambda(
					Actor,
					[Actor]()
					{
						FCrvModule::Get().SelectReference(const_cast<AActor*>(Actor));
					}
				)
			)
		};
	}
	else if (const auto Component = Cast<UActorComponent>(Object))
	{
		const auto Label = FText::FromString(FString::Printf(TEXT("%s"), *Component->GetReadableName()));
		return {
			Component->GetFName(),
			Label,
			FText::FromString(Tooltip),
			FSlateIconFinder::FindIconForClass(Component->GetClass()),
			FUIAction(
				FExecuteAction::CreateWeakLambda(
					Component,
					[Component]()
					{
						FCrvModule::Get().SelectReference(const_cast<UActorComponent*>(Component));
					}
				)
			)
		};
	}

	return {
		Object->GetFName(),
		FText::FromString(Object->GetName()),
		FText::FromString(Tooltip),
		FSlateIconFinder::FindIconForClass(Object->GetClass()),
		FUIAction()
	};
}

bool IsObjectProperty(const FProperty* InProperty)
{
	return CastField<FObjectPropertyBase>(InProperty) != nullptr;
}

void LogItems(const bool bIsEnabled, const TSet<UObject*>& Items, const int32 IndentLevel = 0)
{
	if (!bIsEnabled) { return; }
	static TSet<UObject*> LastItems;
	const auto Indent = FString::ChrN(IndentLevel + 1, TEXT('\t'));
	int32 Index = 0;
	UE_CLOG(bIsEnabled, LogCrv, Log, TEXT("%sItems [Num: %d, Prev %d]:\n"), *Indent, Items.Num(), LastItems.Num());
	const bool bIsSame = Items.Num() == LastItems.Num() && LastItems.Difference(Items).Num() == 0 && Items.Difference(LastItems).Num() == 0;
	if (bIsSame)
	{
		// same, do not log all elements
		UE_CLOG(bIsEnabled, LogCrv, Log, TEXT("%s\tSame as Previous\n"), *Indent);
		return;
	}

	LastItems = Items;
	for (const auto Item : Items)
	{
		UE_CLOG(bIsEnabled, LogCrv, Log, TEXT("\t%d%s%s\n"), Index++, *Indent, *GetDebugName(Item));
	}
}

template <typename T>
void TryAddValue(TSet<UObject*>& Found, T PropValue)
{
	if (!PropValue.IsValid()) { return; }
	Found.Add(PropValue.Get());
}

TArray<UObject*> FindSoftObjectReferences(const UObject* RootObject)
{
	if (!IsValid(RootObject)) { return {}; }
	const auto Settings = GetDefault<UCrvSettings>();
	TSet<UObject*> Found;
	if (Settings->bWalkObjectProperties && RootObject->GetClass())
	{
		for (TFieldIterator<FObjectPropertyBase> PropIt(RootObject->GetClass(), EFieldIterationFlags::IncludeSuper); PropIt; ++PropIt)
		{
			if (const auto SoftObjectProperty = CastField<FSoftObjectProperty>(*PropIt))
			{
				TryAddValue(Found, SoftObjectProperty->GetPropertyValue_InContainer(RootObject));
			}
			else if (const auto WeakObjectProperty = CastField<FWeakObjectProperty>(*PropIt))
			{
				TryAddValue(Found, WeakObjectProperty->GetPropertyValue_InContainer(RootObject));
			}
			else if (const auto LazyObjectProperty = CastField<FLazyObjectProperty>(*PropIt))
			{
				TryAddValue(Found, LazyObjectProperty->GetPropertyValue_InContainer(RootObject));
			}
		}
	}
	return Found.Array();
}

bool FCrvRefSearch::CanDisplayReference(const UObject* RootObject, const UObject* LeafObject)
{
	if (!IsValid(LeafObject))
	{
		return false;
	}
	if (!IsValid(RootObject))
	{
		return false;
	}
	if (LeafObject == RootObject)
	{
		return false;
	}
	if (LeafObject->IsA<ULevel>())
	{
		return false;
	}
	if (LeafObject->IsA<UWorld>())
	{
		return false;
	}
	if (LeafObject->IsA<UClass>())
	{
		return false;
	}
	if (LeafObject->IsA<UStreamableRenderAsset>())
	{
		return false;
	}
	if (LeafObject->IsA<UReferenceVisualizerComponent>())
	{
		return false;
	}

	const auto CrvSettings = GetDefault<UCrvSettings>();

	// hide references to self...
	if (LeafObject->IsInOuter(RootObject))
	{
		// unless it's an actor (e.g. child actor)
		if (!LeafObject->IsA<AActor>())
		{
			return false;
		}
	}

	if (CrvSettings->bIgnoreTransient && LeafObject->HasAnyFlags(RF_Transient))
	{
		return false;
	}
	if (CrvSettings->bIgnoreArchetype && LeafObject->HasAnyFlags(RF_ArchetypeObject))
	{
		return false;
	}
	if (CrvSettings->TargetSettings.IgnoreReferencesToClasses.Contains(LeafObject->GetClass()))
	{
		return false;
	}
	if (LeafObject->IsA<AActor>())
	{
		return true;
	}

	if (LeafObject->IsA<UActorComponent>())
	{
		return CrvSettings->bShowComponents;
	}

	return CrvSettings->bShowObjects;
}

FCrvSet FCrvRefSearch::GetSelectionSet()
{
	TArray<UObject*> SelectedActors;
	TArray<UObject*> SelectedComponents;
	GEditor->GetSelectedActors()->GetSelectedObjects(SelectedActors);
	GEditor->GetSelectedComponents()->GetSelectedObjects(SelectedComponents);
	TSet<UObject*> Selected;
	Selected.Append(SelectedActors);
	Selected.Append(SelectedComponents);
	
	FCrvSet SelectedSet;
	SelectedSet.Append(Selected);
	return MoveTemp(SelectedSet);
}

static auto GetCanDisplayReference(UObject* RootObject)
{
	return [RootObject](const UObject* LeafObject)
	{
		return FCrvRefSearch::CanDisplayReference(RootObject, LeafObject);
	};
}

static void FilterRefs(UObject* RootObject, TSet<UObject*>& LeafObjects)
{
	auto* const CrvSettings = GetDefault<UCrvSettings>();
	const auto bDebugEnabled = CrvSettings->bDebugEnabled;
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("FilterRefs LeafObjects >> [Num: %d]"), LeafObjects.Num());
	LeafObjects.Remove(RootObject);
	const auto Current = LeafObjects;
	LeafObjects.Reset(); // keeps allocations
	LeafObjects.Append(
		Current.Array().FilterByPredicate(GetCanDisplayReference(RootObject))
	);
	LeafObjects.Compact();
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("FilterRefs LeafObjects << [Num: %d]"), LeafObjects.Num());
};

void FCrvRefSearch::FindRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, const bool bIsOutgoing)
{
	if (bIsOutgoing)
	{
		FindOutRefs(RootObject, LeafObjects, Visited);
	}
	else
	{
		FindInRefs(RootObject, LeafObjects, Visited);
	}
}

void FCrvRefSearch::FindOutRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, uint32 Depth)
{
	if (!IsValid(RootObject))
	{
		return;
	}
	auto* const CrvSettings = GetDefault<UCrvSettings>();
	const auto bDebugEnabled = CrvSettings->bDebugEnabled;
	const auto Indent = FString::ChrN(Depth * 1, TEXT('\t'));

	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("%sFinding FindOutRefs for '%s' [Num %d] >>"), *Indent, *GetDebugName(RootObject), LeafObjects.Num());

	TSet<UObject*> TargetObjects;
	TargetObjects.Add(RootObject);
	TargetObjects.Append(Search::FindOwnedObjects(RootObject));

	const auto SearchIndent = FString::ChrN(Depth + 1, TEXT('\t'));
	for (const auto Object : TargetObjects)
	{
		if (!Object) { continue; }
		if (CrvSettings->TargetSettings.IgnoreReferencesToClasses.Contains(Object->GetClass()))
		{
			return;
		}

		TArray<UObject*> NewItemsArray;
		FReferenceFinder RefFinder(
			NewItemsArray,
			nullptr,
			false,
			CrvSettings->bIgnoreArchetype,
			false,
			CrvSettings->bIgnoreTransient
		);
		RefFinder.FindReferences(Object);

		auto NewItems = TSet(NewItemsArray);
		if (CrvSettings->bWalkObjectProperties)
		{
			auto NewSoftItems = FindSoftObjectReferences(Object);
			NewItems.Append(NewSoftItems);
		}
		NewItems = TSet(NewItems.Array().FilterByPredicate(GetCanDisplayReference(RootObject)));
		NewItems = NewItems.Difference(LeafObjects);
		LeafObjects.Append(NewItems);
	}

	const auto BeforeFiltering = LeafObjects.Num();
	FilterRefs(RootObject, LeafObjects);
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("%sFinding FindOutRefs for '%s' << [BeforeFiltering: %d Num: %d]"), *Indent, *GetDebugName(RootObject), BeforeFiltering, LeafObjects.Num());
	LogItems(bDebugEnabled, LeafObjects, Depth);
}

bool ChainContains(const FReferenceChainSearch::FReferenceChain* Chain, const UObject* Object)
{
	if (!Chain)
	{
		return false;
	}
	if (const auto Root = Chain->GetRootNode()->ObjectInfo->TryResolveObject())
	{
		if (Root == Object)
		{
			return true;
		}
	}

	for (int32 i = 0; i < Chain->Num(); i++)
	{
		if (Chain->GetNode(i)->ObjectInfo->TryResolveObject() == Object)
		{
			return true;
		}
	}
	return false;
}

FReferenceChainSearch::FGraphNode* FCrvRefSearch::FindGraphNode(TArray<FReferenceChainSearch::FReferenceChain*> Chains, const UObject* Target)
{
	for (const auto Chain : Chains)
	{
		for (int32 i = 0; i < Chain->Num(); i++)
		{
			const auto GraphNode = Chain->GetNode(i);
			const auto Object = GraphNode->ObjectInfo->TryResolveObject();
			if (Object == Target)
			{
				return GraphNode;
			}
		}
	}
	return nullptr;
}

void FCrvRefSearch::FindInRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, const uint32 Depth)
{
	if (!IsValid(RootObject))
	{
		UE_CLOG(GetDefault<UCrvSettings>()->bDebugEnabled, LogCrv, Warning, TEXT("FindInRefs: Invalid RootObject"));
		return;
	}
	const auto Indent = FString::ChrN(Depth * 1, TEXT('|'));
	const auto CrvSettings = GetMutableDefault<UCrvSettings>();
	const auto SearchSettings = DuplicateObject(CrvSettings, GetTransientPackage());
	// disable recursion for incoming references, too expensive
	SearchSettings->bIsRecursive = true;
	SearchSettings->bWalkObjectProperties = false;
	UE_CLOG(SearchSettings->bDebugEnabled, LogCrv, Log, TEXT("%sFinding Incoming References for '%s' >>"), *Indent, *GetDebugName(RootObject));
	const auto bDebugEnabled = SearchSettings->bDebugEnabled;
	const auto SearchIndent = FString::ChrN(Depth + 1, TEXT('\t'));
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("%s FindInRefs: %s"), *SearchIndent, *GetDebugName(RootObject));

	const EReferenceChainSearchMode SearchMode = SearchSettings->GetSearchModeIncoming();
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("%sFindInRefs: %s [Mode: %s]"), *SearchIndent, *GetDebugName(RootObject), *LexToString(SearchMode));
	TSet<UObject*> TargetObjects;
	TargetObjects.Add(RootObject);
	TargetObjects.Append(Search::FindOwnedObjects(RootObject));

	const FReferenceChainSearch RefChainSearch(TargetObjects.Array(), SearchMode);
	auto ReferenceChains = RefChainSearch.GetReferenceChains();
	for (auto TargetObject : TargetObjects)
	{
		auto GraphNode = FindGraphNode(ReferenceChains, TargetObject);
		if (!GraphNode)
		{
			UE_CLOG(bDebugEnabled, LogCrv, Error, TEXT("%sFindInRefs: No Graph Node for '%s'"), *SearchIndent, *GetDebugName(RootObject));
			continue;
		}
		Visited.Add(TargetObject);
		for (const auto InRef : GraphNode->ReferencedByObjects)
		{
			if (auto InRefObject = InRef->ObjectInfo->TryResolveObject())
			{
				UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("%sIncoming Reference: %s"), *SearchIndent, *GetDebugName(InRefObject));
				if (!Visited.Contains(InRefObject))
				{
					Visited.Add(InRefObject);
					if (CanDisplayReference(RootObject, InRefObject))
					{
						LeafObjects.Add(InRefObject);
					}
				}
			}
		}
	}

	FilterRefs(RootObject, LeafObjects);
	UE_CLOG(CrvSettings->bDebugEnabled, LogCrv, Log, TEXT("%sFinding Incoming References for '%s' [LeafObjects: %d] <<"), *Indent, *GetDebugName(RootObject), LeafObjects.Num());
}
