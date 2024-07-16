#pragma once

#include "CoreMinimal.h"
#include "CrvHitProxy.h"

struct FCrvRefCache
{
	void FillReferences(const UObject* InRootObject, TSet<TWeakObjectPtr<>>& LinkedObjects, bool bIsOutgoing);

	void FillCache(TSet<TObjectPtr<UObject>> InRootObjects, const bool bMultiple);
	void Invalidate(const FString& Reason);

	TMap<TWeakObjectPtr<UObject>, TSet<TWeakObjectPtr<UObject>>> Outgoing;
	TMap<TWeakObjectPtr<UObject>, TSet<TWeakObjectPtr<UObject>>> Incoming;

	TSet<TWeakObjectPtr<UObject>> RootObjects;
	bool bCached = false;
	bool bHadValidItems = false;

	TMap<TObjectPtr<UObject>, TSet<TObjectPtr<UObject>>> GetValidCached(bool bIsOutgoing);
};

struct FCrvHitProxyRef
{
	FHitProxyId Id;
	TWeakObjectPtr<const UObject> LeafObject;
	TWeakObjectPtr<const UObject> RootObject;

	bool IsValid() const
	{
		return Id != FHitProxyId() && LeafObject.IsValid() && RootObject.IsValid();
	}

	FCrvHitProxyRef(HCrvHitProxy* HitProxy)
		: Id(HitProxy->Id),
		LeafObject(HitProxy->LeafObject),
		RootObject(HitProxy->RootObject) {}
};
