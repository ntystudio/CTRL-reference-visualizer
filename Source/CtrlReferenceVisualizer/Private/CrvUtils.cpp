#include "CrvUtils.h"

FString CRV::GetDebugName(const UObject* Object)
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


AActor* CRV::GetOwner(UObject* Object)
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
