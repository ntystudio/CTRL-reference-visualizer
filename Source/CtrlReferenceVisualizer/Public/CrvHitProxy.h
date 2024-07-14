
#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

struct CTRLREFERENCEVISUALIZER_API HCrvHitProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();
	const TWeakObjectPtr<const UObject> LeafObject;
	const TWeakObjectPtr<const UObject> RootObject;

	DECLARE_DELEGATE(FOnHitProxyHover)
	FOnHitProxyHover OnHover;

	bool IsValid() const
	{
		return LeafObject.IsValid() && RootObject.IsValid();
	}

	HCrvHitProxy(const UActorComponent* ParentComponent, const UObject* InRootObject, const UObject* InLeafObject)
		: HComponentVisProxy(ParentComponent, HPP_Foreground),
		LeafObject(InLeafObject),
		RootObject(InRootObject)
	{
	}

	virtual EMouseCursor::Type GetMouseCursor() override
	{
		if (OnHover.IsBound())
		{
			OnHover.Execute();
		}
		return EMouseCursor::EyeDropper;
	}
};
