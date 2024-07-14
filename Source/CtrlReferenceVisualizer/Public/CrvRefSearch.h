#pragma once

#include "CoreMinimal.h"
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
	static TSet<UObject*> GetSelectionSet();

	static TArray<UObject*> GetOutRefs(UObject* RootObject);
	static TArray<UObject*> GetInRefs(UObject* RootObject);
	static TSet<UObject*> FindOutRefs(UObject* RootObject);
	static TSet<UObject*> FindInRefs(UObject* RootObject);
	static void FindOutRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, uint32 Depth = 0);

	static TPair<FReferenceChainSearch::FGraphNode*, FReferenceChainSearch::FGraphNode*> FindGraphNodes(TArray<FReferenceChainSearch::FReferenceChain*> Chain, const UObject* From, const UObject* To);
	static FReferenceChainSearch::FGraphNode* FindGraphNode(TArray<FReferenceChainSearch::FReferenceChain*> Chains, const UObject* Target);

	static void FindInRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, const uint32 Depth = 0);
	static TSet<UObject*> FindOutRefs(const UObject* RootObject);
	static TSet<UObject*> FindInRefs(const UObject* RootObject);
	static TArray<UObject*> GetOutRefs(const UObject* RootObject);
	static TArray<UObject*> GetInRefs(const UObject* RootObject);
	static void FindRefs(UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited, const bool bIsOutgoing);
	static TSet<UObject*> FindRefs(UObject* RootObject, bool bIsOutgoing);
	static void FindOutRefs(const UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited);
	static void FindInRefs(const UObject* RootObject, TSet<UObject*>& LeafObjects, TSet<UObject*>& Visited);

	static FCrvMenuItem MakeMenuEntry(const UObject* Parent, const UObject* Object);
	static bool CanDisplayReference(const UObject* RootObject, const UObject* LeafObject);
};
