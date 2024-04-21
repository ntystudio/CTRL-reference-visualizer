#include "CtrlReferenceVisualizer.h"

#include "ISettingsModule.h"
#include "LevelEditorSubsystem.h"
#include "ReferenceVisualizerComponent.h"
#include "CrvCommands.h"
#include "CrvDebugVisualizer.h"
#include "CrvReferenceCollection.h"
#include "CrvSettings.h"
#include "CrvStyle.h"
#include "Selection.h"
#include "UnrealEdGlobals.h"

#include "Editor/UnrealEdEngine.h"

#include "Styling/SlateIconFinder.h"

#include "UObject/Object.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

DEFINE_LOG_CATEGORY(LogCrv);

void FCrvModule::MakeReferenceListSubMenu(UToolMenu* SubMenu, bool bFindOutRefs) const
{
	if (!GetDefault<UCrvSettings>()->bIsEnabled)
	{
		const auto ToolEntry = FToolMenuEntry::InitMenuEntry(
			FName("CtrlReferenceVisualizer_Disabled"),
			LOCTEXT("Disabled", "Disabled"),
			LOCTEXT("DisabledTooltip", "Disabled"),
			FSlateIcon(),
			FUIAction()
		);
		SubMenu->AddMenuEntry(FName("CtrlReferenceVisualizer_Disabled"), ToolEntry);
		return;
	}

	bool bFoundEntry = false;
	TSet<UObject*> Visited;
	for (auto SelectedObject : FCrvRefCollection::GetSelectionSet())
	{
		UE_LOG(LogCrv, Log, TEXT("Find %s for SelectedObject: %s"), *SelectedObject->GetFullName(), bFindOutRefs ? TEXT("Outgoing") : TEXT("Incoming"));
		auto Refs = FCrvRefCollection::FindRefs(SelectedObject, bFindOutRefs);

		auto RefsArray = Refs.Array();
		RefsArray.Sort(
			[](const UObject& A, const UObject& B) { return A.GetFullName().Compare(B.GetFullName()) < 0; }
		);
		for (auto Ref : RefsArray)
		{
			if (Visited.Contains(Ref))
			{
				continue;
			}
			FToolMenuSection& Section = SubMenu->FindOrAddSection(FName(Ref->GetClass()->GetPathName()));
			FToolMenuSection* SectionPtr = &Section;
			if (auto BP = UBlueprint::GetBlueprintFromClass(Ref->GetClass()->GetAuthoritativeClass()))
			{
				auto& Section2 = SubMenu->FindOrAddSection(FName(BP->GetFullName()));
				SectionPtr = &Section2;
				// get human readable name from uobject
				SectionPtr->Label = FText::FromString(BP->GetFriendlyName());

			}
			else
			{
				SectionPtr->Label = FText::FromString(Ref->GetClass()->GetName());
			}
			auto [Name, Label, ToolTip, Icon, Action] = FCrvRefCollection::MakeMenuEntry(Ref);
			const auto ToolEntry = FToolMenuEntry::InitMenuEntry(Name, Label, ToolTip, Icon, Action);
			SectionPtr->AddEntry(ToolEntry);
			bFoundEntry = true;
		}
		Visited.Append(Refs);
	}

	if (!bFoundEntry)
	{
		const auto ToolEntry = FToolMenuEntry::InitMenuEntry(
			FName("CtrlReferenceVisualizer_None"),
			LOCTEXT("NoReferencesFound", "No references found"),
			LOCTEXT("NoReferencesFoundTooltip", "No references found"),
			FSlateIcon(),
			FUIAction()
		);
		SubMenu->AddMenuEntry(FName("CtrlReferenceVisualizer_None"), ToolEntry);
	}
}

void FCrvModule::InitActorMenu() const
{
	const auto SubMenu = FToolMenuEntry::InitSubMenu(
		FName("CtrlReferenceVisualizerOutgoing"),
		LOCTEXT("References", "References"),
		LOCTEXT("ReferencesTooltip", "List of actor & component references from selected objects"),
		FNewToolMenuDelegate::CreateLambda(
			[this](UToolMenu* Menu)
			{
				const auto SubMenuOutgoing = FToolMenuEntry::InitSubMenu(
					FName("CtrlReferenceVisualizerOutgoing"),
					LOCTEXT("OutgoingReferences", "Outgoing"),
					LOCTEXT("OutgoingReferencesTooltip", "List of actor & component references from selected objects"),
					FNewToolMenuDelegate::CreateRaw(this, &FCrvModule::MakeReferenceListSubMenu, true),
					FToolUIActionChoice(),
					EUserInterfaceActionType::None,
					false,
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlendSpaceEditor.ArrowRight")
				);
				const auto SubMenuIncoming = FToolMenuEntry::InitSubMenu(
					FName("CtrlReferenceVisualizerIncoming"),
					LOCTEXT("IncomingReferences", "Incoming"),
					LOCTEXT("IncomingReferencesTooltip", "List of incoming actor & component references to selected objects"),
					FNewToolMenuDelegate::CreateRaw(this, &FCrvModule::MakeReferenceListSubMenu, false),
					FToolUIActionChoice(),
					EUserInterfaceActionType::None,
					false,
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlendSpaceEditor.ArrowLeft")
				);
				Menu->AddMenuEntry(SubMenuOutgoing.Name, SubMenuOutgoing);
				Menu->AddMenuEntry(SubMenuIncoming.Name, SubMenuIncoming);
				Menu->AddMenuEntry(FName(), FToolMenuEntry::InitSeparator(NAME_None));
				const auto CanExecuteIfEnabled = FCanExecuteAction::CreateLambda(
					[this]()
					{
						const auto Config = GetDefault<UCrvSettings>();
						return Config->bIsEnabled;
					}
				);
				Menu->AddMenuEntry(
					FName("CtrlReferenceVisualizer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlReferenceVisualizer"),
						LOCTEXT("CtrlReferenceVisualizer", "Enable"),
						LOCTEXT("CtrlReferenceVisualizer_Tooltip", "Toggle Reference Visualizer"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									auto* Settings = GetMutableDefault<UCrvSettings>();
									Settings->SetEnabled(!Settings->bIsEnabled);
									Settings->SaveConfig();
									// get level editor viewport
									auto* LevelEditor = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
									LevelEditor->EditorInvalidateViewports();
								}
							),
							FCanExecuteAction(),
							FIsActionChecked::CreateLambda(
								[this]()
								{
									auto* Settings = GetDefault<UCrvSettings>();
									return Settings->bIsEnabled;
								}
							)
						),
						EUserInterfaceActionType::ToggleButton
					)
				);

				Menu->AddMenuEntry(
					FName("CtrlReferenceVisualizer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlShowIncoming"),
						LOCTEXT("ShowIncoming", "Visualize Incoming"),
						LOCTEXT("ShowIncoming_Tooltip", "Show incoming references as links in the viewport"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									const auto Config = GetMutableDefault<UCrvSettings>();
									Config->SetShowIncomingReferences(!Config->bShowIncomingReferences);
								}
							),
							CanExecuteIfEnabled,
							FIsActionChecked::CreateLambda(
								[this]()
								{
									const auto Config = GetDefault<UCrvSettings>();
									return Config->bShowIncomingReferences;
								}
							)
						),
						EUserInterfaceActionType::ToggleButton
					)
				);
				Menu->AddMenuEntry(
					FName("CtrlReferenceVisualizer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlShowOutgoing"),
						LOCTEXT("ShowOutgoing", "Visualize Outgoing"),
						LOCTEXT("ShowOutgoing_Tooltip", "Show outgoing references as links in the viewport"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									const auto Config = GetMutableDefault<UCrvSettings>();
									Config->SetShowOutgoingReferences(!Config->bShowOutgoingReferences);
								}
							),
							CanExecuteIfEnabled,
							FIsActionChecked::CreateLambda(
								[this]()
								{
									const auto Config = GetDefault<UCrvSettings>();
									return Config->bShowOutgoingReferences;
								}
							)
						),
						EUserInterfaceActionType::ToggleButton
					)
				);

				Menu->AddMenuEntry(
					FName("CtrlReferenceVisualizer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlRefresh"),
						LOCTEXT("Refresh", "Refresh"),
						LOCTEXT("Refresh_Tooltip", "Refresh References"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									const auto Config = GetMutableDefault<UCrvSettings>();
									Config->Refresh();
								}
							),
							CanExecuteIfEnabled
						),
						EUserInterfaceActionType::Button
					)
				);

				Menu->AddMenuEntry(
					FName("CtrlReferenceVisualizerSettings"),
					GetSettingsMenuEntry()
				);
			}
		),
		FToolUIActionChoice(),
		EUserInterfaceActionType::None,
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.ReferenceVisualizer")
	);
	auto* ActorContextMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
	auto&& Section = ActorContextMenu->FindOrAddSection(FName("Ctrl"));
	Section.AddEntry(SubMenu);

}

void FCrvModule::InitLevelMenus() const
{
	const auto Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	auto& Section = Menu->FindOrAddSection("PluginTools");
	if (!Section.FindEntry("CtrlMenuButton"))
	{
		Section.AddEntry(
			FToolMenuEntry::InitComboButton(
				"CtrlMenuButton",
				FToolUIActionChoice(),
				FOnGetContent::CreateLambda(
					[this]() -> TSharedRef<SWidget>
					{
						FToolMenuContext MenuContext;
						return UToolMenus::Get()->GenerateWidget("Ctrl.CtrlMenu", MenuContext);
					}
				),
				LOCTEXT("CtrlMenuEntry", "Ctrl"),
				LOCTEXT("CtrlMenuEntry", "Ctrl Tools"),
				FSlateIcon(
					FCrvStyle::Get()->GetStyleSetName(),
					"Ctrl.TabIcon"
				)
			)
		);
	}

	const auto InNewToolMenu = UToolMenus::Get()->ExtendMenu("Ctrl.CtrlMenu");

	auto ReferenceVisualizerSubMenu = FToolMenuEntry::InitSubMenu(
		"CtrlReferenceVisualizerSubMenu",
		LOCTEXT("CtrlReferenceVisualizerSubMenu", "Reference Visualizer"),
		LOCTEXT("CtrlReferenceVisualizerSubMenu_Tooltip", "Ctrl Reference Visualizer Actions"),
		FNewToolMenuChoice(
			FNewToolMenuDelegate::CreateLambda(
				[this](UToolMenu* Menu)
				{
					auto Section = Menu->AddSection("CtrlReferenceVisualizerSection", LOCTEXT("CtrlReferenceVisualizerSection", "Reference Visualizer"));
					Menu->AddMenuEntry(
						Section.Name,
						FToolMenuEntry::InitMenuEntry(
							FName("CtrlReferenceVisualizer"),
							LOCTEXT("CtrlReferenceVisualizer", "Enable"),
							LOCTEXT("CtrlReferenceVisualizer_Tooltip", "Toggle Reference Visualizer"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateLambda(
									[this]()
									{
										auto* Settings = GetMutableDefault<UCrvSettings>();
										Settings->SetEnabled(!Settings->bIsEnabled);
										Settings->SaveConfig();
										// get level editor viewport
										auto* LevelEditor = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
										LevelEditor->EditorInvalidateViewports();
									}
								),
								FCanExecuteAction(),
								FIsActionChecked::CreateLambda(
									[this]()
									{
										auto* Settings = GetDefault<UCrvSettings>();
										return Settings->bIsEnabled;
									}
								)
							),
							EUserInterfaceActionType::ToggleButton
						)
					);
					Menu->AddMenuEntry(Section.Name, FToolMenuEntry::InitSeparator(NAME_None));
					Menu->AddMenuEntry(Section.Name, GetSettingsMenuEntry());
				}
			)
		)
	);
	ReferenceVisualizerSubMenu.Icon = FSlateIconFinder::FindIcon("BTEditor.Graph.BTNode.Task.RunBehavior.Icon");
	InNewToolMenu->AddMenuEntry("CtrlEditorSubmenu", ReferenceVisualizerSubMenu);
}

FToolMenuEntry FCrvModule::GetSettingsMenuEntry() const
{
	return FToolMenuEntry::InitMenuEntry(
		FName("CtrlReferenceVisualizerSettings"),
		LOCTEXT("CtrlReferenceVisualizerSettings", "Settings"),
		LOCTEXT("CtrlReferenceVisualizerSettings_Tooltip", "Open Reference Visualizer Settings"),
		FSlateIconFinder::FindIcon("LevelViewport.AdvancedSettings"),
		FUIAction(
			FExecuteAction::CreateLambda(
				[this]()
				{
					FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "Ctrl", "Reference Visualizer");
				}
			)
		)
	);
}


void FCrvModule::OnPostEngineInit()
{
	UCrvSettings* Settings = GetMutableDefault<UCrvSettings>();
	SettingsModifiedHandle = Settings->OnModified.AddLambda([this](UObject*, FProperty*) { OnSettingsModified(); });
	Settings->AddComponentClass(UReferenceVisualizerComponent::StaticClass());
	// add visualizer to actors when editor selection is changed
	USelection::SelectionChangedEvent.AddRaw(this, &FCrvModule::OnSelectionChanged);
	RegisterVisualizers();
	FCrvCommands::Register();
	InitActorMenu();
	InitLevelMenus();

}

void FCrvModule::StartupModule()
{
	FCrvStyle::Startup();
	FCrvStyle::Register();

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FCrvModule::OnPostEngineInit);
}

void FCrvModule::DestroyAllCreatedComponents()
{
	for (auto ExistingComponent : CreatedDebugComponents)
	{
		if (ExistingComponent.IsValid())
		{
			ExistingComponent->DestroyComponent();
		}
	}
	CreatedDebugComponents.Empty();
}

void FCrvModule::DestroyStaleDebugComponents(const TArray<AActor*>& CurrentActorSelection)
{
	// destroy any created debug components that are no longer needed
	auto CurrentlyCreatedDebugComponents = CreatedDebugComponents;
	for (auto ExistingComponent : CurrentlyCreatedDebugComponents)
	{
		if (ExistingComponent.IsValid())
		{
			if (auto OwningActor = ExistingComponent->GetOwner())
			{
				if (!CurrentActorSelection.Contains(OwningActor))
				{
					CreatedDebugComponents.Remove(ExistingComponent);
					ExistingComponent->DestroyComponent();
				}
			}
			else
			{
				ExistingComponent->DestroyComponent();
				CreatedDebugComponents.Remove(ExistingComponent);
			}
		}
		else
		{
			CreatedDebugComponents.Remove(ExistingComponent);
		}
	}
}

void FCrvModule::CreateDebugComponentsForActors(TArray<AActor*> Actors)
{
	DestroyStaleDebugComponents(Actors);
	CreatedDebugComponents.Reserve(Actors.Num());
	for (const auto Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			UE_LOG(LogCrv, Warning, TEXT("CreateDebugComponentsForActors: Actor is not valid"));
			continue;
		}
		// don't add debug components to actors that already have them (either auto-created or manually added)
		if (Actor->FindComponentByClass<UReferenceVisualizerComponent>())
		{
			continue;
		}
		
		auto* DebugComponent = Cast<UReferenceVisualizerComponent>(
			Actor->AddComponentByClass(
				UReferenceVisualizerComponent::StaticClass(),
				false,
				FTransform::Identity,
				false
			)
		);
		if (!DebugComponent)
		{
			UE_LOG(LogCrv, Warning, TEXT("CreateDebugComponentsForActors: Failed to add component to actor"));
			continue;
		}
		
		if (!DebugComponent->IsRegistered())
		{
			DebugComponent->RegisterComponent();
		}
		
		if (!CreatedDebugComponents.Contains(DebugComponent))
		{
			CreatedDebugComponents.Add(DebugComponent);
		}
	}
}

bool FCrvModule::IsEnabled()
{
	const auto Settings = GetDefault<UCrvSettings>();
	return Settings->bIsEnabled;
}

void FCrvModule::OnSelectionChanged(UObject* SelectionObject)
{
	static bool bIsReentrant = false;
	if (bIsReentrant) { return; }
	TGuardValue<bool> ReentrantGuard(bIsReentrant, true);
	if (!IsEnabled()) { return; }
	// RegisterVisualizers();
	RefreshSelection(SelectionObject);
}

void FCrvModule::RefreshSelection(UObject* SelectionObject)
{
	static bool bIsReentrant = false;
	if (bIsReentrant) { return; }
	TGuardValue<bool> ReentrantGuard(bIsReentrant, true);
	if (!IsEnabled())
	{
		DestroyAllCreatedComponents();
		return;
	}

	auto const ActorSelection = GEditor->GetSelectedActors();
	
	TArray<AActor*> SelectedActors;
	ActorSelection->GetSelectedObjects<AActor>(SelectedActors);
	auto const Settings = GetDefault<UCrvSettings>();
	UE_CLOG(Settings->bDebugEnabled, LogCrv, Log, TEXT("RefreshSelection: %d actors selected"), SelectedActors.Num());
	CreateDebugComponentsForActors(SelectedActors);
	
	USelection* Selection = Cast<USelection>(SelectionObject);
	if (!Selection) { return; }
	Selection->NoteSelectionChanged();
}

TSharedPtr<FCrvDebugVisualizer> FCrvModule::GetDefaultVisualizer()
{
	return StaticCastSharedPtr<FCrvDebugVisualizer>(
		GUnrealEd->FindComponentVisualizer(
			UReferenceVisualizerComponent::StaticClass()
		)
	);
}

void FCrvModule::RegisterVisualizers()
{
	if (!GUnrealEd) { return; }

	if (!IsEnabled())
	{
		UnregisterVisualizers();
		return;
	}

	// track items to remove.
	// if item is in RegisteredClasses but not in Classes, then it should be removed
	// start with all registered classes, remove as we find them in Classes
	// at the end, all items left in ToRemove should be removed
	const auto* Settings = GetDefault<UCrvSettings>();
	auto ToRemove = RegisteredClasses;
	for (const auto Class : Settings->TargetSettings.TargetComponentClasses)
	{
		if (Class.IsPending())
		{
			ToRemove.Empty(); // do no removing if any class is pending
			UE_LOG(LogCrv, Log, TEXT("RegisterVisualizers: Skipping pending class"));
			continue;
		}
		if (Class.IsNull()) { continue; }
		if (!Class.IsValid()) { continue; }

		auto ClassName = Class->GetFName();
		ToRemove.Remove(ClassName); // do this first so we don't accidentally remove the class later
		// do nothing if class is already registered
		if (GUnrealEd->FindComponentVisualizer(ClassName)) { continue; }

		// don't register classes that are already registered
		if (RegisteredClasses.Contains(ClassName)) { continue; }
		RegisteredClasses.AddUnique(ClassName);
		const auto Visualizer = MakeShared<FCrvDebugVisualizer>();
		GUnrealEd->RegisterComponentVisualizer(ClassName, Visualizer);
		Visualizer->OnRegister();
	}

	// remove any previously registered classes that are no longer registered
	for (const auto ClassName : ToRemove)
	{
		GUnrealEd->UnregisterComponentVisualizer(ClassName);
		RegisteredClasses.Remove(ClassName);
	}
	bDidRegisterVisualizers = true;
}

void FCrvModule::UnregisterVisualizers()
{
	auto CurrentClasses = RegisteredClasses;
	RegisteredClasses.Empty();
	bDidRegisterVisualizers = false;
	if (!GUnrealEd) { return; }
	for (const auto ClassName : CurrentClasses)
	{
		GUnrealEd->UnregisterComponentVisualizer(ClassName);
	}
}


void FCrvModule::Refresh(bool bForceRefresh)
{
	auto* Settings = GetDefault<UCrvSettings>();
	bool bRefreshEnabled = Settings->bRefreshEnabled;
	if (!bRefreshEnabled) { return; }

	static bool bLastEnabled = !IsEnabled(); // force refresh on first call
	const auto bIsEnabled = IsEnabled();
	const auto bEnabledChanged = bLastEnabled != bIsEnabled;
	bLastEnabled = bIsEnabled;
	auto const EditorWorld = GEditor->GetEditorWorldContext().World();
	EditorWorld->GetTimerManager().ClearTimer(RefreshTimerHandle);
	UE_CLOG(Settings->bDebugEnabled, LogCrv, Log, TEXT("Refresh: bForceRefresh: %d, bIsEnabled: %d, bEnabledChanged: %d"), bForceRefresh, bIsEnabled, bEnabledChanged);
	RefreshTimerHandle = EditorWorld->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateLambda(
			[this, bForceRefresh, Settings, bIsEnabled, bEnabledChanged]()
			{
				if (bForceRefresh)
				{
					UnregisterVisualizers();
				}

				bIsEnabled ? RegisterVisualizers() : UnregisterVisualizers();

				if (bEnabledChanged || bForceRefresh)
				{
					RefreshSelection(GEditor->GetSelectedActors());
				}
			}
		)
	);
}

void FCrvModule::ShutdownModule()
{
	if (!UObjectInitialized()) { return; }
	SettingsModifiedHandle.Reset();
	DestroyAllCreatedComponents();
	UnregisterVisualizers();
	FCrvCommands::Unregister();
	FCrvStyle::Shutdown();
}

void FCrvModule::AddTargetComponentClass(UClass* Class)
{
	auto* Settings = GetMutableDefault<UCrvSettings>();
	Settings->AddComponentClass(Class);
}

void FCrvModule::OnSettingsModified()
{
	Refresh(false);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCrvModule, CtrlReferenceVisualizer)
