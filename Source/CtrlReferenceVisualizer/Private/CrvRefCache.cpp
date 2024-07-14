#include "CrvRefCache.h"

#include "CrvRefSearch.h"
#include "CrvSettings.h"
#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

using namespace CRV;

void FCrvRefCache::Invalidate(const FString& Reason)
{
	if (bCached || Outgoing.Num() > 0 || Incoming.Num() > 0)
	{
		const auto bDebugEnabled = GetDefault<UCrvSettings>()->bDebugEnabled;
		UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("Invalidating cache... %s"), *Reason);
	}
	RootObject = nullptr;
	RootComponent = nullptr;
	Outgoing.Reset();
	Incoming.Reset();
	bCached = false;
	bHadValidItems = false;
}

TSet<UObject*> FCrvRefCache::GetValidCached(const bool bIsOutgoing)
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
		Invalidate(FString::Printf(TEXT("Invalid %s references"), bIsOutgoing ? TEXT("Outgoing") : TEXT("Incoming")));
	}
	return Visited;
}

void FCrvRefCache::FillReferences(const UObject* InRootObject, const bool bIsOutgoing)
{
	if (!IsValid(InRootObject))
	{
		Invalidate(FString::Printf(TEXT("FillReferences: Invalid RootObject: %s "), *GetDebugName(InRootObject)));
		return;
	}
	const auto bDebugEnabled = GetDefault<UCrvSettings>()->bDebugEnabled;
	UE_CLOG(bDebugEnabled, LogCrv, Log, TEXT("FillReferences cache... RootObject %s | %s"), *GetDebugName(InRootObject), bIsOutgoing ? TEXT("Outgoing") : TEXT("Incoming"));
	TSet<UObject*> Visited;
	TSet<UObject*> Found;
	FCrvRefSearch::FindRefs(RootObject.Get(), Found, Visited, bIsOutgoing);
	auto&& CachedItems = bIsOutgoing ? Outgoing : Incoming;
	CachedItems.Empty(Found.Num());
	if (RootObject == RootComponent)
	{
		Found.Add(RootComponent->GetOwner());
	}
	for (const auto DstRef : Found)
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

void FCrvRefCache::FillCache(UObject* InRootObject, const bool bMultiple)
{
	if (RootObject != InRootObject)
	{
		Invalidate(FString::Printf(TEXT("FillCache: RootObject Changed %s -> %s"), *GetDebugName(RootObject.Get()), *GetDebugName(InRootObject)));
		RootObject = InRootObject;
	}

	if (!RootObject.IsValid())
	{
		Invalidate(FString::Printf(TEXT("FillCache: Invalid RootObject: %s"), *GetDebugName(RootObject.Get())));
		return;
	}

	if (bCached) { return; }

	const auto Config = GetDefault<UCrvSettings>();
	if (Config->bShowOutgoingReferences)
	{
		FillReferences(RootObject.Get(), true);
	}
	else
	{
		Outgoing.Empty();
	}

	if (Config->bShowIncomingReferences && !bMultiple)
	{
		FillReferences(RootObject.Get(), false);
	}
	else
	{
		Incoming.Empty();
	}

	bCached = true;
}