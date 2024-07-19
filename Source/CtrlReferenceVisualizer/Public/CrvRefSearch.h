#pragma once

#include "CoreMinimal.h"
#include "CrvUtils.h"
#include "UObject/ReferenceChainSearch.h"

using namespace CtrlRefViz;

struct FCrvMenuItem
{
	FName Name;
	FText Label;
	FText ToolTip;
	FSlateIcon Icon;
	FUIAction Action;
};

namespace CtrlRefViz::Search
{
	FString LexToString(const FReferenceChainSearch::FReferenceChain* Chain);
	bool IsExternal(const FReferenceChainSearch::FReferenceChain* Chain);
	FCrvSet FindTargetObjects(UObject* RootObject);
}

class FCrvRefSearch
{
public:
	static FCrvSet GetSelectionSet();

	static void FindOutRefs(FCrvSet RootObjects, FCrvObjectGraph& Graph);
	static void FindInRefs(FCrvSet RootObjects, FCrvObjectGraph& Graph);
	
	static FCrvMenuItem MakeMenuEntry(const UObject* Parent, const UObject* Object);
	static bool CanDisplayReference(const UObject* RootObject, const UObject* LeafObject);
};
