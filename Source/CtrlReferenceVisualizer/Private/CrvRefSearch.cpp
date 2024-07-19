#include "CrvRefSearch.h"

#include "CrvSettings.h"
#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"
#include "ReferenceVisualizerComponent.h"
#include "Selection.h"

#include "Styling/SlateIconFinder.h"

#include "UObject/ReferenceChainSearch.h"
#include "UObject/ReferencerFinder.h"

using namespace CtrlRefViz;

namespace CtrlRefViz::Search
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

	template <typename T>
	void TryAddValue(FCrvSet& Found, T PropValue)
	{
		if (!PropValue.IsValid()) { return; }
		Found.Add(PropValue.Get());
	}

	TArray<UObject*> FindSoftObjectReferences(const UObject* RootObject)
	{
		if (!IsValid(RootObject)) { return {}; }
		const auto Settings = GetDefault<UCrvSettings>();
		FCrvSet Found;
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

	FCrvSet FindOwnedObjects(FCrvSet Targets)
	{
		const auto CrvSettings = GetDefault<UCrvSettings>();
		TArray<UObject*> Owned;
		for (const auto Target : Targets)
		{
			FReferenceFinder RefFinder(
				Owned,
				Target,
				false,
				CrvSettings->bIgnoreArchetype,
				CrvSettings->bIsRecursive,
				CrvSettings->bIgnoreTransient
			);
			RefFinder.FindReferences(Target);
		}

		return TSet(Owned);
	}

	FCrvSet FindOwnedObjects(UObject* Target)
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

#pragma region GetObjectFlagsString
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
#pragma endregion GetObjectFlagsString
}

FCrvMenuItem FCrvRefSearch::MakeMenuEntry(const UObject* Parent, const UObject* Object)
{
	static FCrvMenuItem Empty;
	if (!Object)
	{
		return Empty;
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

#pragma region CanDisplayReference
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
#pragma endregion CanDisplayReference

FCrvSet FCrvRefSearch::GetSelectionSet()
{
	TArray<UObject*> SelectedActors;
	TArray<UObject*> SelectedComponents;
	GEditor->GetSelectedActors()->GetSelectedObjects(SelectedActors);
	GEditor->GetSelectedComponents()->GetSelectedObjects(SelectedComponents);
	FCrvSet Selected;
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

FCrvSet Search::FindTargetObjects(UObject* RootObject)
{
	static FCrvSet Empty;
	if (!IsValid(RootObject)) { return Empty; }
	FCrvSet TargetObjects;
	TargetObjects.Append(Search::FindOwnedObjects(RootObject));
	TargetObjects.Add(RootObject);
	return MoveTemp(TargetObjects);
}

void FCrvRefSearch::FindOutRefs(FCrvSet RootObjects, FCrvObjectGraph& Graph)
{
	auto* const CrvSettings = GetDefault<UCrvSettings>();
	Graph.Reserve(RootObjects.Num());
	Graph.Reset();
	for (auto RootObject : RootObjects)
	{
		FCrvSet RootObjectReferences;
		const auto TargetObjects = Search::FindTargetObjects(RootObject);
		for (const auto TargetObject : TargetObjects)
		{
			TArray<UObject*> NewItemsArray;
			FReferenceFinder RefFinder(
				NewItemsArray,
				nullptr,
				false,
				CrvSettings->bIgnoreArchetype,
				CrvSettings->bIsRecursive,
				CrvSettings->bIgnoreTransient
			);
			RefFinder.FindReferences(TargetObject);
			RootObjectReferences.Append(NewItemsArray.FilterByPredicate(GetCanDisplayReference(RootObject)));
		}
		for (const auto TargetObject : TargetObjects)
		{
			if (CrvSettings->bWalkObjectProperties)
			{
				RootObjectReferences.Append(Search::FindSoftObjectReferences(TargetObject).FilterByPredicate(GetCanDisplayReference(RootObject)));
			}
		}
		Graph.Add(RootObject, RootObjectReferences);
	}
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

void FCrvRefSearch::FindInRefs(FCrvSet RootObjects, FCrvObjectGraph& Graph)
{
	Graph.Reserve(RootObjects.Num());
	for (auto RootObject : RootObjects)
	{
		auto TargetObjects = Search::FindTargetObjects(RootObject);
		auto Referencers = FReferencerFinder::GetAllReferencers(TargetObjects, nullptr, EReferencerFinderFlags::SkipInnerReferences);
		auto Filtered = Referencers.FilterByPredicate(GetCanDisplayReference(RootObject));
		Graph.Add(RootObject, TSet(Filtered));
	}
}