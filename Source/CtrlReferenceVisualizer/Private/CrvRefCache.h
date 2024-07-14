#pragma once

#include "CoreMinimal.h"
#include "CrvHitProxy.h"

struct FCrvRefCache
{
	void FillReferences(const UObject* InRootObject, bool bIsOutgoing);
	void FillCache(UObject* InRootObject, bool bMultiple);
	void Invalidate(const FString& Reason);

	TSet<TWeakObjectPtr<UObject>> Outgoing;
	TSet<TWeakObjectPtr<UObject>> Incoming;
	TWeakObjectPtr<UObject> RootObject;
	TWeakObjectPtr<const UActorComponent> RootComponent;
	bool bCached = false;
	bool bHadValidItems = false;

	TSet<UObject*> GetValidCached(bool bIsOutgoing);
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
