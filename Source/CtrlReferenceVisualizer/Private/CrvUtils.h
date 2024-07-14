#pragma once
#include "CoreMinimal.h"

#define CRV_LOG_LINE FString::Printf(TEXT("%s:%d"), *FPaths::GetCleanFilename(ANSI_TO_TCHAR(__FILE__)), __LINE__)

namespace CRV
{
	FString GetDebugName(const UObject* Object);
	AActor* GetOwner(UObject* Object);
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
