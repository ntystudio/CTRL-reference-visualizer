#pragma once

#include "CoreMinimal.h"

struct FLrvMenuItem
{
	FName Name;
	FText Label;
	FText ToolTip;
	FSlateIcon Icon;
	FUIAction Action;
};

class FLrvRefCollection
{
public:
	static TSet<UObject*> GetSelectionSet();
	
	static TArray<UObject*> GetOutRefs(UObject* Target);
	static TArray<UObject*> GetInRefs(UObject* Target);
	static TSet<UObject*> FindOutRefs(UObject* Target);
	static TSet<UObject*> FindInRefs(UObject* Target);
	static void FindOutRefs(UObject* Target, TSet<UObject*>& Visited);
	static void FindInRefs(UObject* Target, TSet<UObject*>& Visited);
	static TSet<UObject*> FindOutRefs(const UObject* Target);
	static TSet<UObject*> FindInRefs(const UObject* Target);
	static TArray<UObject*> GetOutRefs(const UObject* Target);
	static TArray<UObject*> GetInRefs(const UObject* Target);
	static void FindRefs(UObject* Target, TSet<UObject*>& Visited, bool bIsOutgoing);
	static TSet<UObject*> FindRefs(UObject* Target, bool bIsOutgoing);
	static void FindOutRefs(const UObject* Target, TSet<UObject*>& Visited);
	static void FindInRefs(const UObject* Target, TSet<UObject*>& Visited);
	
	static FLrvMenuItem MakeMenuEntry(const UObject* Object);
	static bool CanDisplayReference(const UObject* Object);
};
