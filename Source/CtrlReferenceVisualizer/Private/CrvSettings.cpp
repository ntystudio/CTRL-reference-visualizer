// Fill out your copyright notice in the Description page of Project Settings.

#include "CrvSettings.h"

#include "CrvUtils.h"
#include "CtrlReferenceVisualizer.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

FCrvLineStyle FCrvLineStyle::DefaultOutgoingLineStyle = {
	// LineStyle
	ECrvLineType::Arrow,
	// LineColor
	FColor::Green,
	// LineColorComponent
	FColor::Emerald,
	// LineColorObject
	FColor::Cyan,
	// LineThickness
	2.f,
	// ArrowSize
	10.f,
	// Depth Priority
	SDPG_Foreground
};

FCrvLineStyle FCrvLineStyle::DefaultIncomingLineStyle = {
	// LineStyle
	ECrvLineType::Dash,
	// LineColor
	FColor::Blue,
	// LineColorComponent
	(FLinearColor(FColor::Emerald) * FLinearColor::Blue).ToFColor(true),
	// LineColorObject
	(FLinearColor(FColor::Cyan) * FLinearColor::Blue).ToFColor(true),
	// LineThickness
	1.f,
	// ArrowSize
	5.f,
	// Depth Priority
	SDPG_Foreground
};

FCrvStyleSettings::FCrvStyleSettings()
{
}

FCrvTargetSettings::FCrvTargetSettings()
	: IgnoreReferencesToClasses({}),
	TargetComponentClasses({TSoftClassPtr<UActorComponent>(UReferenceVisualizerComponent::StaticClass())})
{

}

UCrvSettings::UCrvSettings()
{
	CategoryName = "Ctrl";
	SectionName = "Reference Visualizer";
	Help = LOCTEXT("Help", "Shows references between actors and components in the level.");
}

FText UCrvSettings::GetSectionText() const
{
	return LOCTEXT("CtrlReferenceVisualizerSettingsName", "Reference Visualizer");
}

FText UCrvSettings::GetSectionDescription() const
{
	return LOCTEXT("CtrlReferenceVisualizerSettingsDescription", "Configure Ctrl Reference Visualizer Plugin");
}

void UCrvSettings::AddComponentClass(const TSoftClassPtr<UActorComponent>& ComponentClass)
{
	TargetSettings.TargetComponentClasses.AddUnique(ComponentClass);
	UpdateTargets();
}

void UCrvSettings::SetEnabled(bool bNewEnabled)
{
	if (bIsEnabled == bNewEnabled) { return; }
	bIsEnabled = bNewEnabled;
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UCrvSettings, bIsEnabled)
	);
	auto Event = FPropertyChangedEvent(Property);
	PostEditChangeProperty(Event);
}

void UCrvSettings::ToggleEnabled()
{
	SetEnabled(!bIsEnabled);
}

void UCrvSettings::SetShowIncomingReferences(bool bNewShowIncomingReferences)
{
	if (bShowIncomingReferences == bNewShowIncomingReferences) { return; }

	bShowIncomingReferences = bNewShowIncomingReferences;
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UCrvSettings, bShowIncomingReferences)
	);
	auto Event = FPropertyChangedEvent(Property);
	PostEditChangeProperty(Event);
}

void UCrvSettings::SetShowOutgoingReferences(bool bNewShowOutgoingReferences)
{
	if (bShowOutgoingReferences == bNewShowOutgoingReferences) { return; }
	bShowOutgoingReferences = bNewShowOutgoingReferences;
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UCrvSettings, bShowOutgoingReferences)
	);
	auto Event = FPropertyChangedEvent(Property);
	PostEditChangeProperty(Event);
}

void UCrvSettings::Refresh()
{
	UpdateTargets();
}

void UCrvSettings::UpdateTargets()
{
	if (bIsUpdating) { return; }
	TGuardValue UpdateGuard(bIsUpdating, true);
	CleanTargets();
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UCrvSettings, TargetSettings)
	);
	OnModified.Broadcast(this, Property);
}

void UCrvSettings::CleanTargets()
{
	// only allow at most one null entry
	auto OnlyOneNull = [](auto IsNullFunc)
	{
		bool bFoundOneNull = false;
		auto Fn = [&bFoundOneNull, &IsNullFunc](const auto& Item)
		{
			auto bIsOk = !IsNullFunc(Item);
			if (!bIsOk && !bFoundOneNull)
			{
				bFoundOneNull = true;
				return false;
			}
			return !bIsOk;
		};
		return Fn;
	};

	// clean target component classes
	TargetSettings.TargetComponentClasses.RemoveAll(
		OnlyOneNull([](const TSoftClassPtr<UActorComponent>& ClassPtr) { return ClassPtr.IsNull(); })
	);
}

bool UCrvSettings::IsEnabled() const
{
	return bIsEnabled;
}

bool UCrvSettings::GetAutoAddComponents() const
{
	return IsEnabled() && bAutoAddComponents;
}

// Function to open plugin documentation
void OpenPluginDocumentation(const FString& PluginName)
{
	// Construct the path to the plugin descriptor file
	const FString PluginPath = FPaths::ProjectPluginsDir() / PluginName / FString::Printf(TEXT("%s.uplugin"), *PluginName);

	// Read the plugin descriptor file
	FString FileContents;
	const bool bDidLoadFile = FFileHelper::LoadFileToString(FileContents, *PluginPath);
	if (!bDidLoadFile)
	{
		UE_LOG(LogCrv, Warning, TEXT("Failed to load plugin descriptor for %s"), *PluginName);
		return;
	}
	// Parse the JSON content
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	const bool bDidDeserialize = FJsonSerializer::Deserialize(Reader, JsonObject);
	if (!bDidDeserialize)
	{
		UE_LOG(LogCrv, Warning, TEXT("Failed to deserialize plugin descriptor for %s"), *PluginName);
		return;
	}
	// Extract the DocsURL
	FString DocsURL;
	const bool Success = JsonObject->TryGetStringField(TEXT("DocsURL"), DocsURL);
	if (!Success)
	{
		UE_LOG(LogCrv, Warning, TEXT("No DocsURL found in plugin descriptor for %s"), *PluginName);
		return;
	}
	// Open the documentation URL in the default web browser
	FPlatformProcess::LaunchURL(*DocsURL, nullptr, nullptr);
}

void UCrvSettings::Documentation() const
{
	OpenPluginDocumentation(TEXT("CtrlReferenceVisualizer"));
}

void UCrvSettings::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITOR
	IConsoleManager::Get().RegisterConsoleVariableSink_Handle(
		FConsoleCommandDelegate::CreateUObject(this, &UCrvSettings::CVarSink)
	);
#endif
	UpdateTargets();
}

#if WITH_EDITOR

FCrvLineStyle UCrvSettings::GetLineStyle(const ECrvDirection Direction) const
{
	return Direction == ECrvDirection::Incoming ? Style.LineStyleIncoming : Style.LineStyleOutgoing;
}

void UCrvSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!PropertyChangedEvent.Property) { return; }

	UpdateTargets();
	OnModified.Broadcast(this, PropertyChangedEvent.Property);
}

void UCrvSettings::CVarSink()
{
	if (IsTemplate())
	{
		ImportConsoleVariableValues();
		SetEnabled(CrvConsoleVars::CVarReferenceVisualizerEnabled->GetBool());
		UpdateTargets();
	}
}
#endif
#undef LOCTEXT_NAMESPACE
