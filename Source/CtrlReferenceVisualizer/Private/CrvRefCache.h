#pragma once

#include "CoreMinimal.h"
#include "CrvHitProxy.h"
#include "CrvSettings.h"
#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"
#include "CrvRefCache.generated.h"

class UReferenceVisualizerComponent;
using namespace CtrlRefViz;

UCLASS(Transient, Hidden)
class UCrvRefCache : public UObject
{
	GENERATED_BODY()
public:
	static FCrvSet GenerateRootObjects();
	static FCrvSet GenerateAllRootObjects();

	bool HasValues() const;
	bool Contains(const UObject* Object) const;

	void FillCache(const FCrvSet& InRootObjects);
	void Reset(const FString& String);
	// reset + schedule update
	void Invalidate(const FString& Reason);

	// Get all incoming/outgoing references for a given object
	TSet<UObject*> GetReferences(const UObject* Object, ECrvDirection Direction);
	FCrvObjectGraph GetValidCached(ECrvDirection Direction);

	void UpdateCache();
	void ScheduleUpdate();

	FCrvWeakGraph Outgoing;
	FCrvWeakGraph Incoming;
	// Objects we want to find references for
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<UObject>> WeakRootObjects;
	void AutoAddComponents(const FCrvSet& InRootObjects);

	bool bCached = false;
	bool bHadValidItems = false;

	DECLARE_MULTICAST_DELEGATE(FOnCacheUpdated)
	FOnCacheUpdated OnCacheUpdated;
protected:
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<UReferenceVisualizerComponent>> AutoCreatedComponents;
private:
	FTimerHandle UpdateCacheNextTickHandle;
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
