#pragma once
#include "CoreMinimal.h"

#define CRV_LOG_LINE FString::Printf(TEXT("%s:%d"), *FPaths::GetCleanFilename(ANSI_TO_TCHAR(__FILE__)), __LINE__)

namespace CtrlRefViz
{
	FString GetDebugName(const UObject* Object);
	AActor* GetOwner(UObject* Object);
	AActor* GetOwner(const UObject* Object);
}

namespace CrvConsoleVars
{
	static int32 IsEnabled = 1;
	static FAutoConsoleVariableRef CVarReferenceVisualizerEnabled(
		TEXT("ctrl.ReferenceVisualizer"),
		IsEnabled,
		TEXT("Enable Actor Reference Visualizer Level Editor Display")
	);
}

namespace CtrlRefViz
{
	using FCrvSet = TSet<UObject*>;
	using FCrvSetProp = TSet<TObjectPtr<UObject>>;
	using FCrvWeakSet = TSet<TWeakObjectPtr<UObject>>;
	using FCrvObjectGraph = TMap<TObjectPtr<UObject>, FCrvSet>;
	using FCrvWeakGraph = TMap<TWeakObjectPtr<UObject>, FCrvWeakSet>;

	template <typename T>
	bool AreSetsEqual(const T& Set, const T& RHS)
	{
		return Set.Num() == RHS.Num() && Set.Difference(RHS).Num() == 0 && RHS.Difference(Set).Num() == 0;
	}

	template <typename T = UObject>
	TSet<TWeakObjectPtr<T>> ToWeakSet(const TSet<T*>& In)
	{
		static TSet<TWeakObjectPtr<T>> EmptySet;
		if (In.IsEmpty()) { return EmptySet; }
		FCrvWeakSet Out;
		Out.Reserve(In.Num());
		for (const auto Object : In)
		{
			if (!IsValid(Object)) { continue; }
			Out.Add(Object);
		}
		return MoveTemp(Out);
	}

	template <typename T = UObject>
	TSet<T*> ResolveWeakSet(const TSet<TWeakObjectPtr<T>>& Set, bool& bFoundInvalid)
	{
		static TSet<T*> EmptySet;
		bFoundInvalid = false;
		if (Set.IsEmpty()) { return EmptySet; }
		FCrvSet ValidPointers;
		// ValidPointers.Reserve(Set.Num());
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

	template <typename T = UObject>
	TSet<T*> ResolveWeakSet(const TSet<TWeakObjectPtr<T>>& Set)
	{
		bool bFoundInvalid = false;
		return ResolveWeakSet(Set, bFoundInvalid);
	}

	inline FCrvObjectGraph ResolveWeakGraph(const FCrvWeakGraph& Graph, bool& bFoundInvalid)
	{
		bFoundInvalid = false;
		static FCrvObjectGraph EmptyGraph;
		if (Graph.IsEmpty()) { return EmptyGraph; }
		FCrvObjectGraph ValidItems;
		ValidItems.Reserve(Graph.Num());
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

	inline FCrvObjectGraph ResolveWeakGraph(const FCrvWeakGraph& Graph)
	{
		bool bFoundInvalid = false;
		return ResolveWeakGraph(Graph, bFoundInvalid);
	}
	
	inline FCrvWeakGraph ToWeakGraph(const FCrvObjectGraph& Graph)
	{
		static FCrvWeakGraph EmptyGraph;
		if (Graph.IsEmpty()) { return EmptyGraph; }
		FCrvWeakGraph Out;
		Out.Reserve(Graph.Num());
		for (const auto [Object, Set] : Graph)
		{
			if (!IsValid(Object)) { continue; }
			Out.Add(Object, ToWeakSet(Set));
		}
		return MoveTemp(Out);
	}
}
