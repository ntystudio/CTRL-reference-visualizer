// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReferenceVisualizerComponent.generated.h"


UCLASS(Transient, Hidden, HideDropdown, NotBlueprintable)
class CTRLREFERENCEVISUALIZER_API UReferenceVisualizerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReferenceVisualizerComponent() { PrimaryComponentTick.bCanEverTick = false; }
};
