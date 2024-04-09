// Fill out your copyright notice in the Description page of Project Settings.

#include "LrvSettings.h"
#include "CtrlLevelReferenceViewer.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "LevelReferenceViewer"

FLrvStyleSettings::FLrvStyleSettings()
{
}

FLrvTargetSettings::FLrvTargetSettings()
	: IgnoreReferencesToClasses({}),
	  TargetComponentClasses({TSoftClassPtr<UActorComponent>(ULevelReferenceViewerComponent::StaticClass())})
{

}

ULrvSettings::ULrvSettings()
{
	CategoryName = "Ctrl";
	SectionName = "Level Reference Viewer";
	Help = LOCTEXT("Help", "Shows references between actors and components in the level.");
}

FText ULrvSettings::GetSectionText() const
{
	return LOCTEXT("CtrlLevelReferenceViewerSettingsName", "Level Reference Viewer");
}

FText ULrvSettings::GetSectionDescription() const
{
	return LOCTEXT("CtrlLevelReferenceViewerSettingsDescription", "Configure Ctrl Level Reference Viewer Plugin");
}

void ULrvSettings::AddComponentClass(const TSoftClassPtr<UActorComponent>& ComponentClass)
{
	TargetSettings.TargetComponentClasses.AddUnique(ComponentClass);
	UpdateTargets();
}

void ULrvSettings::SetEnabled(bool bNewEnabled)
{
	if (bIsEnabled == bNewEnabled) { return; }
	bIsEnabled = bNewEnabled;
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(ULrvSettings, bIsEnabled)
	);
	auto Event = FPropertyChangedEvent(Property);
	PostEditChangeProperty(Event);
}

void ULrvSettings::ToggleEnabled()
{
	SetEnabled(!bIsEnabled);
}

void ULrvSettings::SetShowIncomingReferences(bool bNewShowIncomingReferences)
{
	if (bShowIncomingReferences == bNewShowIncomingReferences) { return; }

	bShowIncomingReferences = bNewShowIncomingReferences;
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(ULrvSettings, bShowIncomingReferences)
	);
	auto Event = FPropertyChangedEvent(Property);
	PostEditChangeProperty(Event);
}

void ULrvSettings::SetShowOutgoingReferences(bool bNewShowOutgoingReferences)
{
	if (bShowOutgoingReferences == bNewShowOutgoingReferences) { return; }
	bShowOutgoingReferences = bNewShowOutgoingReferences;
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(ULrvSettings, bShowOutgoingReferences)
	);
	auto Event = FPropertyChangedEvent(Property);
	PostEditChangeProperty(Event);
}

void ULrvSettings::Refresh()
{
	UpdateTargets();
}

void ULrvSettings::UpdateTargets()
{
	if (bIsUpdating) { return; }
	TGuardValue UpdateGuard(bIsUpdating, true);
	CleanTargets();
	const auto Property = StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(ULrvSettings, TargetSettings)
	);
	OnModified.Broadcast(this, Property);
}

void ULrvSettings::CleanTargets()
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

// Function to open plugin documentation
void OpenPluginDocumentation(const FString& PluginName)
{
	// Construct the path to the plugin descriptor file
	FString const PluginPath = FPaths::ProjectPluginsDir() / PluginName / FString::Printf(TEXT("%s.uplugin"), *PluginName);

	// Read the plugin descriptor file
	FString FileContents;
	bool const bDidLoadFile = FFileHelper::LoadFileToString(FileContents, *PluginPath);
	if (!bDidLoadFile)
	{
		UE_LOG(LogLrv, Warning, TEXT("Failed to load plugin descriptor for %s"), *PluginName);
		return;
	}
	// Parse the JSON content
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> const Reader = TJsonReaderFactory<>::Create(FileContents);
	bool const bDidDeserialize = FJsonSerializer::Deserialize(Reader, JsonObject);
	if (!bDidDeserialize)
	{
		UE_LOG(LogLrv, Warning, TEXT("Failed to deserialize plugin descriptor for %s"), *PluginName);
		return;
	}
	// Extract the DocsURL
	FString DocsURL;
	bool const Success = JsonObject->TryGetStringField(TEXT("DocsURL"), DocsURL);
	if (!Success)
	{
		UE_LOG(LogLrv, Warning, TEXT("No DocsURL found in plugin descriptor for %s"), *PluginName);
		return;
	}
	// Open the documentation URL in the default web browser
	FPlatformProcess::LaunchURL(*DocsURL, nullptr, nullptr);
}

void ULrvSettings::Documentation() const
{
	OpenPluginDocumentation(TEXT("CtrlLevelReferenceViewer"));
}

void ULrvSettings::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITOR
	IConsoleManager::Get().RegisterConsoleVariableSink_Handle(
		FConsoleCommandDelegate::CreateUObject(this, &ULrvSettings::CVarSink)
	);
#endif
	UpdateTargets();
}

#if WITH_EDITOR
void ULrvSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!PropertyChangedEvent.Property) { return; }

	UpdateTargets();
	OnModified.Broadcast(this, PropertyChangedEvent.Property);
}

void ULrvSettings::CVarSink()
{
	if (IsTemplate())
	{
		ImportConsoleVariableValues();
		SetEnabled(LrvConsoleVars::CVarLevelReferenceViewerEnabled->GetBool());
		UpdateTargets();
	}
}
#endif
#undef LOCTEXT_NAMESPACE
