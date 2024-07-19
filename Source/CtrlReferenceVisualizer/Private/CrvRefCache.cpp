#include "CrvRefCache.h"

#include "CrvRefSearch.h"
#include "CrvSettings.h"
#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"
#include "Algo/AnyOf.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

using namespace CtrlRefViz;

bool UCrvRefCache::HasValues() const
{
	return WeakRootObjects.Num() > 0 || Outgoing.Num() > 0 || Incoming.Num() > 0;
}

void UCrvRefCache::Reset(const FString& Reason)
{
	bCached = false;
	bHadValidItems = false;
	WeakRootObjects.Reset();
	Outgoing.Reset();
	Incoming.Reset();
	UE_CLOG(FCrvModule::IsDebugEnabled(), LogCrv, Log, TEXT("Cache reset... %s"), *Reason);
}

void UCrvRefCache::Invalidate(const FString& Reason)
{
	UE_CLOG(FCrvModule::IsDebugEnabled(), LogCrv, Log, TEXT("Invalidating cache..."));
	Reset(Reason);
	ScheduleUpdate();
}

bool UCrvRefCache::Contains(const UObject* Object) const
{
	return WeakRootObjects.Contains(Object);
}

TSet<UObject*> UCrvRefCache::GetReferences(const UObject* Object, const ECrvDirection Direction)
{
	static TSet<UObject*> Empty;
	if (!Contains(Object)) { return Empty; }
	const auto Cached = GetValidCached(Direction);
	if (const auto Found = Cached.Find(Object))
	{
		return *Found;
	} else
	{
		return Empty;
	}
}

FCrvSet UCrvRefCache::GenerateAllRootObjects()
{
	FCrvSet All;
	for (TObjectIterator<UReferenceVisualizerComponent> It; It; ++It)
	{
		if (const auto Item = *It)
		{
			if (Item->HasAnyFlags(RF_ClassDefaultObject))
			{
				continue;
			}
			if (const auto Owner = Item->GetOwner())
			{
				All.Add(Owner);
			}
		}
	}
	return MoveTemp(All);
}

FCrvSet UCrvRefCache::GenerateRootObjects()
{
	static FCrvSet Empty;
	switch (GetDefault<UCrvSettings>()->Mode)
	{
		case ECrvMode::OnlySelected:
		{
			return FCrvRefSearch::GetSelectionSet();
		}
		case ECrvMode::SelectedOrAll:
		{
			auto SelectionSet = FCrvRefSearch::GetSelectionSet();
			if (SelectionSet.Num() > 0)
			{
				return MoveTemp(SelectionSet);
			}
			return GenerateAllRootObjects();
		}
		case ECrvMode::All:
		{
			return GenerateAllRootObjects();
		}
		default:
		{
			return Empty;
		}
	}
}

void UCrvRefCache::UpdateCache()
{
	GEditor->GetTimerManager()->ClearTimer(UpdateCacheNextTickHandle);
	FillCache(GenerateRootObjects());
}

void UCrvRefCache::ScheduleUpdate()
{
	GEditor->GetTimerManager()->ClearTimer(UpdateCacheNextTickHandle);
	auto WeakThis = TWeakObjectPtr<UCrvRefCache>(this);
	UpdateCacheNextTickHandle = GEditor->GetTimerManager()->SetTimerForNextTick([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->UpdateCache();
		}
	});
}

FCrvObjectGraph UCrvRefCache::GetValidCached(const ECrvDirection Direction)
{
	static FCrvObjectGraph Empty;
	if (!bCached) { return Empty; }

	const auto Cached = Direction == ECrvDirection::Outgoing ? Outgoing : Incoming;
	bool bFoundInvalid = false;
	auto ValidCached = ResolveWeakGraph(Cached, bFoundInvalid);
	if (bFoundInvalid)
	{
		Invalidate(FString::Printf(TEXT("Invalid %s references"), Direction == ECrvDirection::Outgoing ? TEXT("Outgoing") : TEXT("Incoming")));
	}
	return MoveTemp(ValidCached);
}


bool HasValidItems(const FCrvWeakGraph& CachedItems)
{
	for (const auto& [WeakObject, Items] : CachedItems)
	{
		if (!WeakObject.IsValid())
		{
			continue;
		}

		if (Items.Num() > 0)
		{
			if (Algo::AnyOf(Items, [](const TWeakObjectPtr<UObject>& Item) { return Item.IsValid(); }))
			{
				return true;
			}
		}
	}
	return false;
}

void UCrvRefCache::AutoAddComponents(const FCrvSet& InRootObjects)
{
	const auto bAutoAddComponents = GetDefault<UCrvSettings>()->GetAutoAddComponents();
	if (!bAutoAddComponents && AutoCreatedComponents.Num() == 0) { return; }
	// Remove Components
	{
		TSet<UReferenceVisualizerComponent*> ToRemove;
		auto CurrentAutoCreatedComponents = AutoCreatedComponents;
		for (auto Item : CurrentAutoCreatedComponents)
		{
			if (!Item.IsValid())
			{
				AutoCreatedComponents.Remove(Item);
				bCached = false;
				continue;
			}

			if (!bAutoAddComponents || !InRootObjects.Contains(Item->GetOwner()))
			{
				AutoCreatedComponents.Remove(Item);
				bCached = false;
				ToRemove.Add(Item.Get());
			}
		}

		for (const auto Item : ToRemove)
		{
			const auto Owner = Item->GetOwner();
			Owner->RemoveOwnedComponent(Item);
			if (IsValid(Item))
			{
				Item->DestroyComponent();
			}
		}
	}

	if (!bAutoAddComponents) { return; }
	auto AddComponent = [this](AActor* Actor)
	{
		if (Actor->FindComponentByClass<UReferenceVisualizerComponent>())
		{
			return; // already has component
		}
		bCached = false;
		auto* DebugComponent = Cast<UReferenceVisualizerComponent>(
			Actor->AddComponentByClass(
				UReferenceVisualizerComponent::StaticClass(),
				false,
				FTransform::Identity,
				false
			)
		);

		if (!DebugComponent) { return; }
		AutoCreatedComponents.Add(DebugComponent);
		Actor->AddOwnedComponent(DebugComponent);
		if (!DebugComponent->IsRegistered())
		{
			DebugComponent->RegisterComponent();
		}
	};

	for (const auto Item : InRootObjects)
	{
		ensure(Item != nullptr);
		if (const auto Actor = Cast<AActor>(Item))
		{
			AddComponent(Actor);
		}
		else if (auto OuterActor = Item->GetTypedOuter<AActor>())
		{
			AddComponent(OuterActor);
		}
	}
}

void UCrvRefCache::FillCache(const FCrvSet& InRootObjects)
{
	if (bCached && !AreSetsEqual(ResolveWeakSet(WeakRootObjects), InRootObjects))
	{
		Reset(FString::Printf(TEXT("FillCache: RootObjects Changed")));
	}

	const auto Config = GetDefault<UCrvSettings>();
	AutoAddComponents(InRootObjects);
	WeakRootObjects = ToWeakSet(InRootObjects);
	WeakRootObjects.Compact();
	const auto RootObjects = InRootObjects;

	if (bCached)
	{
		UE_CLOG(FCrvModule::IsDebugEnabled(), LogCrv, Log, TEXT("Cache already filled. RootObjects: %d, Outgoing: %d, Incoming: %d"), RootObjects.Num(), Outgoing.Num(), Incoming.Num());
		return;
	}

	if (Config->bShowOutgoingReferences)
	{
		FCrvObjectGraph OutRefs;
		FCrvRefSearch::FindOutRefs(RootObjects, OutRefs);
		Outgoing = ToWeakGraph(OutRefs);
		Outgoing.Compact();
	}
	else
	{
		Outgoing.Reset();
	}

	if (Config->bShowIncomingReferences)
	{
		FCrvObjectGraph InRefs;
		FCrvRefSearch::FindInRefs(RootObjects, InRefs);
		Incoming = ToWeakGraph(InRefs);
		Incoming.Compact();
	}
	else
	{
		Incoming.Reset();
	}

	if (!bHadValidItems)
	{
		bHadValidItems = HasValidItems(Outgoing) || HasValidItems(Incoming);
	}

	if (OnCacheUpdated.IsBound())
	{
		OnCacheUpdated.Broadcast();
	}

	bCached = HasValues();
	UE_CLOG(FCrvModule::IsDebugEnabled(), LogCrv, Log, TEXT("Cache filled: RootObjects: %d, Outgoing: %d, Incoming: %d"), RootObjects.Num(), Outgoing.Num(), Incoming.Num());
}
