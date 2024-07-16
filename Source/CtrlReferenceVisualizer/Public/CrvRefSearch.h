#pragma once

#include "CoreMinimal.h"
#include "CtrlReferenceVisualizer.h"
#include "UObject/ReferenceChainSearch.h"

struct FCrvMenuItem
{
	FName Name;
	FText Label;
	FText ToolTip;
	FSlateIcon Icon;
	FUIAction Action;
};

struct FCrvRefChainNode
{
	TWeakObjectPtr<UObject> Referencer;
	TWeakObjectPtr<UObject> Object;
	FName ReferencerName;
	FReferenceChainSearch::EReferenceType Type;
};

namespace CRV::Search
{
	FString LexToString(const FReferenceChainSearch::FReferenceChain* Chain);
	bool IsExternal(const FReferenceChainSearch::FReferenceChain* Chain);
}

class FCrvRefSearch
{
public:
	static FCrvSet GetSelectionSet();

	static void FindOutRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, uint32 Depth = 0);
	static void FindInRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, const uint32 Depth = 0);
	static void FindRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, const bool bIsOutgoing);
	
	static FReferenceChainSearch::FGraphNode* FindGraphNode(TArray<FReferenceChainSearch::FReferenceChain*> Chains, const UObject* Target);
	static FCrvMenuItem MakeMenuEntry(const UObject* Parent, const UObject* Object);
	static bool CanDisplayReference(const UObject* RootObject, const UObject* LeafObject);
};
