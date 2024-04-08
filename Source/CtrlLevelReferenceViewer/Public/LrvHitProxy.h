
#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

struct CTRLLEVELREFERENCEVIEWER_API HLrvHitProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();
	const TWeakObjectPtr<const UObject> TargetObject;
	bool bIsActor = true;

	DECLARE_DELEGATE(FOnHitProxyHover);
	FOnHitProxyHover OnHover;

	bool IsValid() const
	{
		return TargetObject.IsValid();
	}
	
	const UActorComponent* GetComponent() const
	{
		return !bIsActor ? Cast<UActorComponent>(TargetObject.Get()) : nullptr;
	}
	
	const AActor* GetActor() const
	{
		return bIsActor ? Cast<AActor>(TargetObject.Get()) : nullptr;
	}
	
	const AActor* GetOwner() const
	{
		if (!IsValid()) { return nullptr; }
		return bIsActor ? GetActor() : GetComponent()->GetOwner();
	}

	HLrvHitProxy(const UActorComponent* ParentComponent, const AActor* InTargetActor)
		: HComponentVisProxy(ParentComponent, HPP_Foreground),
		  TargetObject(InTargetActor)
	{
	}

	HLrvHitProxy(const UActorComponent* ParentComponent, const UActorComponent* InTargetComponent)
		: HComponentVisProxy(ParentComponent, HPP_Foreground),
		  TargetObject(InTargetComponent),
		  bIsActor(false)
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
