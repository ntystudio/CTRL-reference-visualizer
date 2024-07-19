#include "CrvUtils.h"

FString CtrlRefViz::GetDebugName(const UObject* Object)
{
	if (!Object)
	{
		return TEXT("null");
	}
	FString Name = GetNameSafe(Object);
	if (Object->IsA<AActor>())
	{
		Name = Cast<AActor>(Object)->GetActorNameOrLabel();
	}
	if (Object->IsA<UActorComponent>())
	{
		Name = Cast<UActorComponent>(Object)->GetReadableName();
	}
	return FString::Printf(TEXT("<%s>%s"), *GetNameSafe(Object->GetClass()), *Name);
}

AActor* CtrlRefViz::GetOwner(const UObject* Object)
{
	return GetOwner(const_cast<UObject*>(Object));
}

AActor* CtrlRefViz::GetOwner(UObject* Object)
{
	if (!Object) { return nullptr; }
	if (const auto Actor = Cast<AActor>(Object))
	{
		return Actor;
	}
	if (const auto Component = Cast<UActorComponent>(Object))
	{
		return Component->GetOwner();
	}
	return Object->GetTypedOuter<AActor>();
}
