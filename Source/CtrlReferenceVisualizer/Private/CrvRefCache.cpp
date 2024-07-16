#include "CrvRefCache.h"

#include "CrvRefSearch.h"
#include "CrvSettings.h"
#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"
#include "Algo/AnyOf.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

using namespace CRV;

void FCrvRefCache::Invalidate(const FString& Reason)
{
	if (bCached || Outgoing.Num() > 0 || Incoming.Num() > 0)
	{
		const auto bDebugEnabled = GetDefault<UCrvSettings>()->bDebugEnabled;
		UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("Invalidating cache... %s"), *Reason);
	}
	RootObjects.Empty();
	Outgoing.Reset();
	Incoming.Reset();
	bCached = false;
	bHadValidItems = false;
}

FCrvWeakSet ToWeakSet(FCrvSet& In)
{
	FCrvWeakSet Out;
	Out.Reserve(In.Num());
	for (const auto Object : In)
	{
		Out.Add(Object);
	}
	return MoveTemp(Out);
}

FCrvSet ResolveWeakSet(const FCrvWeakSet& Set, bool& bFoundInvalid)
{
	FCrvSet ValidPointers;
	ValidPointers.Reserve(Set.Num());
	bFoundInvalid = false;
	for (const auto LinkedObject : Set)
	{
		if (LinkedObject.IsValid())
		{
			ValidPointers.Add(LinkedObject.Get());
		}
		else
		{
			bFoundInvalid = true;
		}
	}
	return MoveTemp(ValidPointers);
}

FCrvSet ResolveWeakSet(const FCrvWeakSet& Set)
{
	bool bFoundInvalid = false;
	return ResolveWeakSet(Set, bFoundInvalid);
}

FCrvObjectGraph ResolveWeakMap(const FCrvWeakObjectGraph& Graph, bool& bFoundInvalid)
{
	FCrvObjectGraph ValidItems;
	ValidItems.Reserve(Graph.Num());
	bFoundInvalid = false;
	for (const auto [Object, WeakSetPtrs] : Graph)
	{
		if (!Object.IsValid())
		{
			bFoundInvalid = true;
			continue;
		}
		ValidItems.Add(Object.Get(), ResolveWeakSet(WeakSetPtrs));
	}
	return MoveTemp(ValidItems);
}

FCrvObjectGraph ResolveWeakMap(const FCrvWeakObjectGraph& Graph)
{
	bool bFoundInvalid = false;
	return ResolveWeakMap(Graph, bFoundInvalid);
}

FCrvObjectGraph FCrvRefCache::GetValidCached(const bool bIsOutgoing)
{
	if (!bCached)
	{
		return {};
	}
	const auto Cached = bIsOutgoing ? Outgoing : Incoming;
	bool bFoundInvalid = false;
	auto ValidCached = ResolveWeakMap(Cached, bFoundInvalid);
	if (bFoundInvalid)
	{
		Invalidate(FString::Printf(TEXT("Invalid %s references"), bIsOutgoing ? TEXT("Outgoing") : TEXT("Incoming")));
	}
	return MoveTemp(ValidCached);
}

void FCrvRefCache::FillReferences(const UObject* InRootObject, FCrvWeakSet& LinkedObjects, bool bIsOutgoing)
{
	if (!IsValid(InRootObject))
	{
		Invalidate(FString::Printf(TEXT("FillReferences: Invalid RootObject: %s "), *GetDebugName(InRootObject)));
		return;
	}
	const auto bDebugEnabled = GetDefault<UCrvSettings>()->bDebugEnabled;
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("FillReferences cache... RootObject %s | %s"), *GetDebugName(InRootObject), bIsOutgoing ? TEXT("Outgoing") : TEXT("Incoming"));
	const auto RootObject = const_cast<UObject*>(InRootObject);
	TSet<UObject*> Visited;
	TSet<UObject*> Found;
	FCrvRefSearch::FindRefs(RootObject, Found, Visited, bIsOutgoing);
	LinkedObjects.Reserve(LinkedObjects.Num() + Found.Num());
	for (const auto LinkedObject : Found)
	{
		LinkedObjects.Add(LinkedObject);
	}
	LinkedObjects.Compact();
}

template <typename T>
bool AreSetsEqual(const T& Set, const T& RHS)
{
	return Set.Num() == RHS.Num() && Set.Difference(RHS).Num() == 0 && RHS.Difference(Set).Num() == 0;
}

bool HasValidItems(const FCrvWeakObjectGraph& CachedItems)
{
	for (const auto& [WeakObject, Items] : CachedItems)
	{
		if (WeakObject.IsValid())
		{
			if (Items.Num() > 0)
			{
				return Algo::AnyOf(Items, [](const TWeakObjectPtr<UObject>& Item)
				{
					return Item.IsValid();
				});
			}
		}
	}
	return false;
}

void FCrvRefCache::FillCache(FCrvSet InRootObjects, const bool bMultiple)
{
	if (!AreSetsEqual(ResolveWeakSet(RootObjects), InRootObjects))
	{
		Invalidate(FString::Printf(TEXT("FillCache: RootObjects Changed")));
		RootObjects = ToWeakSet(InRootObjects);
	}

	for (const auto RootObject : RootObjects)
	{
		if (!RootObject.IsValid())
		{
			Invalidate(FString::Printf(TEXT("FillCache: Invalid RootObject: %s"), *GetDebugName(RootObject.Get())));
			return;
		}
	}

	if (bCached)
	{
		return;
	}

	const auto Config = GetDefault<UCrvSettings>();
	const auto bDebugEnabled = Config->bDebugEnabled;
	for (const auto RootObjectWeak : RootObjects)
	{
		if (Config->bShowOutgoingReferences)
		{
			FillReferences(RootObjectWeak.Get(), Outgoing.FindOrAdd(RootObjectWeak), true);
		}
		else
		{
			Outgoing.Empty();
		}

		if (Config->bShowIncomingReferences && !bMultiple)
		{
			FillReferences(RootObjectWeak.Get(), Incoming.FindOrAdd(RootObjectWeak), false);
		}
		else
		{
			Incoming.Empty();
		}
	}

	if (!bHadValidItems)
	{
		// UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("FillReferences Cache has %d valid items."), CachedItems.Num());
		bHadValidItems = HasValidItems(Outgoing) || HasValidItems(Incoming);
	}

	bCached = true;
}
