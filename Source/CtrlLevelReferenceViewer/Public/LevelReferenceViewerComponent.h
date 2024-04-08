// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelReferenceViewerComponent.generated.h"


UCLASS(Transient, Hidden, HideDropdown, NotBlueprintable)
class CTRLLEVELREFERENCEVIEWER_API ULevelReferenceViewerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULevelReferenceViewerComponent() { PrimaryComponentTick.bCanEverTick = false; }
};
