#include "CtrlLevelReferenceViewer.h"

#include "DelayAction.h"
#include "ISettingsModule.h"
#include "LevelEditorSubsystem.h"
#include "LrvCommands.h"
#include "LevelReferenceViewerComponent.h"
#include "LrvDebugVisualizer.h"
#include "LrvReferenceCollection.h"
#include "LrvSettings.h"
#include "LrvStyle.h"
#include "Selection.h"
#include "UnrealEdGlobals.h"

#include "Editor/UnrealEdEngine.h"

#include "Styling/SlateIconFinder.h"

#include "UObject/Object.h"

#define LOCTEXT_NAMESPACE "LevelReferenceViewer"

DEFINE_LOG_CATEGORY(LogLrv);

void FLrvModule::MakeReferenceListSubMenu(UToolMenu* SubMenu, bool bFindOutRefs) const
{
	if (!GetDefault<ULrvSettings>()->bIsEnabled)
	{
		const auto ToolEntry = FToolMenuEntry::InitMenuEntry(
			FName("CtrlLevelReferenceViewer_Disabled"),
			LOCTEXT("Disabled", "Disabled"),
			LOCTEXT("DisabledTooltip", "Disabled"),
			FSlateIcon(),
			FUIAction()
		);
		SubMenu->AddMenuEntry(FName("CtrlLevelReferenceViewer_Disabled"), ToolEntry);
		return;
	}

	bool bFoundEntry = false;
	TSet<UObject*> Visited;
	for (auto SelectedObject : FLrvRefCollection::GetSelectionSet())
	{
		UE_LOG(LogLrv, Log, TEXT("Find %s for SelectedObject: %s"), *SelectedObject->GetFullName(), bFindOutRefs ? TEXT("Outgoing") : TEXT("Incoming"));
		auto Refs = FLrvRefCollection::FindRefs(SelectedObject, bFindOutRefs);

		// auto Refs = bFindOutRefs
		// 			? FLrvRefCollection::FindOutRefs(SelectedObject)
		// 			: FLrvRefCollection::FindInRefs(SelectedObject);
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
			auto [Name, Label, ToolTip, Icon, Action] = FLrvRefCollection::MakeMenuEntry(Ref);
			const auto ToolEntry = FToolMenuEntry::InitMenuEntry(Name, Label, ToolTip, Icon, Action);
			SectionPtr->AddEntry(ToolEntry);
			bFoundEntry = true;
		}
		Visited.Append(Refs);
	}

	if (!bFoundEntry)
	{
		const auto ToolEntry = FToolMenuEntry::InitMenuEntry(
			FName("CtrlLevelReferenceViewer_None"),
			LOCTEXT("NoReferencesFound", "No references found"),
			LOCTEXT("NoReferencesFoundTooltip", "No references found"),
			FSlateIcon(),
			FUIAction()
		);
		SubMenu->AddMenuEntry(FName("CtrlLevelReferenceViewer_None"), ToolEntry);
	}
}

void FLrvModule::InitActorMenu() const
{
	const auto SubMenu = FToolMenuEntry::InitSubMenu(
		FName("CtrlLevelReferenceViewerOutgoing"),
		LOCTEXT("References", "References"),
		LOCTEXT("ReferencesTooltip", "List of actor & component references from selected objects"),
		FNewToolMenuDelegate::CreateLambda(
			[this](UToolMenu* Menu)
			{
				const auto SubMenuOutgoing = FToolMenuEntry::InitSubMenu(
					FName("CtrlLevelReferenceViewerOutgoing"),
					LOCTEXT("OutgoingReferences", "Outgoing"),
					LOCTEXT("OutgoingReferencesTooltip", "List of actor & component references from selected objects"),
					FNewToolMenuDelegate::CreateRaw(this, &FLrvModule::MakeReferenceListSubMenu, true),
					FToolUIActionChoice(),
					EUserInterfaceActionType::None,
					false,
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlendSpaceEditor.ArrowRight")
				);
				const auto SubMenuIncoming = FToolMenuEntry::InitSubMenu(
					FName("CtrlLevelReferenceViewerIncoming"),
					LOCTEXT("IncomingReferences", "Incoming"),
					LOCTEXT("IncomingReferencesTooltip", "List of incoming actor & component references to selected objects"),
					FNewToolMenuDelegate::CreateRaw(this, &FLrvModule::MakeReferenceListSubMenu, false),
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
						const auto Config = GetDefault<ULrvSettings>();
						return Config->bIsEnabled;
					}
				);
				Menu->AddMenuEntry(
					FName("CtrlLevelReferenceViewer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlLevelReferenceViewer"),
						LOCTEXT("CtrlLevelReferenceViewer", "Enable"),
						LOCTEXT("CtrlLevelReferenceViewer_Tooltip", "Toggle Level Reference Viewer"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									auto* Settings = GetMutableDefault<ULrvSettings>();
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
									auto* Settings = GetDefault<ULrvSettings>();
									return Settings->bIsEnabled;
								}
							)
						),
						EUserInterfaceActionType::ToggleButton
					)
				);

				Menu->AddMenuEntry(
					FName("CtrlLevelReferenceViewer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlShowIncoming"),
						LOCTEXT("ShowIncoming", "Visualize Incoming"),
						LOCTEXT("ShowIncoming_Tooltip", "Show incoming references as links in the viewport"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									const auto Config = GetMutableDefault<ULrvSettings>();
									Config->SetShowIncomingReferences(!Config->bShowIncomingReferences);
								}
							),
							CanExecuteIfEnabled,
							FIsActionChecked::CreateLambda(
								[this]()
								{
									const auto Config = GetDefault<ULrvSettings>();
									return Config->bShowIncomingReferences;
								}
							)
						),
						EUserInterfaceActionType::ToggleButton
					)
				);
				Menu->AddMenuEntry(
					FName("CtrlLevelReferenceViewer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlShowOutgoing"),
						LOCTEXT("ShowOutgoing", "Visualize Outgoing"),
						LOCTEXT("ShowOutgoing_Tooltip", "Show outgoing references as links in the viewport"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									const auto Config = GetMutableDefault<ULrvSettings>();
									Config->SetShowOutgoingReferences(!Config->bShowOutgoingReferences);
								}
							),
							CanExecuteIfEnabled,
							FIsActionChecked::CreateLambda(
								[this]()
								{
									const auto Config = GetDefault<ULrvSettings>();
									return Config->bShowOutgoingReferences;
								}
							)
						),
						EUserInterfaceActionType::ToggleButton
					)
				);

				Menu->AddMenuEntry(
					FName("CtrlLevelReferenceViewer"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlRefresh"),
						LOCTEXT("Refresh", "Refresh"),
						LOCTEXT("Refresh_Tooltip", "Refresh References"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									const auto Config = GetMutableDefault<ULrvSettings>();
									Config->Refresh();
								}
							),
							CanExecuteIfEnabled
						),
						EUserInterfaceActionType::Button
					)
				);

				Menu->AddMenuEntry(
					FName("CtrlLevelReferenceViewerSettings"),
					FToolMenuEntry::InitMenuEntry(
						FName("CtrlLevelReferenceViewerSettings"),
						LOCTEXT("CtrlLevelReferenceViewerSettings", "Settings"),
						LOCTEXT("CtrlLevelReferenceViewerSettings_Tooltip", "Open Level Reference Viewer Settings"),
						FSlateIconFinder::FindIcon("LevelViewport.AdvancedSettings"),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this]()
								{
									FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "Ctrl", "Level Reference Viewer");
								}
							)
						)
					)
				);
			}
		),
		FToolUIActionChoice(),
		EUserInterfaceActionType::None,
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.ReferenceViewer")
	);
	auto* ActorContextMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
	auto&& Section = ActorContextMenu->FindOrAddSection(FName("Ctrl"));
	Section.AddEntry(SubMenu);

}

void FLrvModule::InitLevelMenus() const
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
					FLrvStyle::Get()->GetStyleSetName(),
					"Ctrl.TabIcon"
				)
			)
		);
	}

	const auto InNewToolMenu = UToolMenus::Get()->ExtendMenu("Ctrl.CtrlMenu");

	auto ReferenceViewerSubMenu = FToolMenuEntry::InitSubMenu(
		"CtrlLevelReferenceViewerSubMenu",
		LOCTEXT("CtrlLevelReferenceViewerSubMenu", "Level Reference Viewer"),
		LOCTEXT("CtrlLevelReferenceViewerSubMenu_Tooltip", "Ctrl Level Reference Viewer Actions"),
		FNewToolMenuChoice(
			FNewToolMenuDelegate::CreateLambda(
				[this](UToolMenu* Menu)
				{
					auto Section = Menu->AddSection("CtrlLevelReferenceViewerSection", LOCTEXT("CtrlLevelReferenceViewerSection", "Level Reference Viewer"));
					Menu->AddMenuEntry(
						Section.Name,
						FToolMenuEntry::InitMenuEntry(
							FName("CtrlLevelReferenceViewer"),
							LOCTEXT("CtrlLevelReferenceViewer", "Enable"),
							LOCTEXT("CtrlLevelReferenceViewer_Tooltip", "Toggle Level Reference Viewer"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateLambda(
									[this]()
									{
										auto* Settings = GetMutableDefault<ULrvSettings>();
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
										auto* Settings = GetDefault<ULrvSettings>();
										return Settings->bIsEnabled;
									}
								)
							),
							EUserInterfaceActionType::ToggleButton
						)
					);
					Menu->AddMenuEntry(Section.Name, FToolMenuEntry::InitSeparator(NAME_None));
					Menu->AddMenuEntry(
						Section.Name,
						FToolMenuEntry::InitMenuEntry(
							FName("CtrlLevelReferenceViewerSettings"),
							LOCTEXT("CtrlLevelReferenceViewerSettings", "Settings"),
							LOCTEXT("CtrlLevelReferenceViewerSettings_Tooltip", "Open Level Reference Viewer Settings"),
							FSlateIconFinder::FindIcon("LevelViewport.AdvancedSettings"),
							FUIAction(
								FExecuteAction::CreateLambda(
									[this]()
									{
										FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "Ctrl", "Level Reference Viewer");
									}
								)
							)
						)
					);
				}
			)
		)
	);
	ReferenceViewerSubMenu.Icon = FSlateIconFinder::FindIcon("BTEditor.Graph.BTNode.Task.RunBehavior.Icon");
	InNewToolMenu->AddMenuEntry("CtrlEditorSubmenu", ReferenceViewerSubMenu);
}

void FLrvModule::OnPostEngineInit()
{
	ULrvSettings* Settings = GetMutableDefault<ULrvSettings>();
	SettingsModifiedHandle = Settings->OnModified.AddLambda([this](UObject*, FProperty*) { OnSettingsModified(); });
	Settings->AddComponentClass(ULevelReferenceViewerComponent::StaticClass());
	// add visualizer to actors when editor selection is changed
	USelection::SelectionChangedEvent.AddRaw(this, &FLrvModule::OnSelectionChanged);
	RegisterVisualizers();
	FLrvCommands::Register();
	InitActorMenu();
	InitLevelMenus();

}

void FLrvModule::StartupModule()
{
	FLrvStyle::Startup();
	FLrvStyle::Register();

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FLrvModule::OnPostEngineInit);
}

void FLrvModule::DestroyAllCreatedComponents()
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

void FLrvModule::DestroyStaleDebugComponents(const TArray<AActor*>& CurrentActorSelection)
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

void FLrvModule::CreateDebugComponentsForActors(TArray<AActor*> Actors)
{
	DestroyStaleDebugComponents(Actors);
	CreatedDebugComponents.Reserve(Actors.Num());
	for (const auto Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			UE_LOG(LogLrv, Warning, TEXT("CreateDebugComponentsForActors: Actor is not valid"));
			continue;
		}
		// don't add debug components to actors that already have them (either auto-created or manually added)
		if (Actor->FindComponentByClass<ULevelReferenceViewerComponent>())
		{
			continue;
		}
		
		auto* DebugComponent = Cast<ULevelReferenceViewerComponent>(
			Actor->AddComponentByClass(
				ULevelReferenceViewerComponent::StaticClass(),
				false,
				FTransform::Identity,
				false
			)
		);
		if (!DebugComponent)
		{
			UE_LOG(LogLrv, Warning, TEXT("CreateDebugComponentsForActors: Failed to add component to actor"));
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

bool FLrvModule::IsEnabled()
{
	const auto Settings = GetDefault<ULrvSettings>();
	return Settings->bIsEnabled;
}

void FLrvModule::OnSelectionChanged(UObject* SelectionObject)
{
	static bool bIsReentrant = false;
	if (bIsReentrant) { return; }
	TGuardValue<bool> ReentrantGuard(bIsReentrant, true);
	if (!IsEnabled()) { return; }
	// RegisterVisualizers();
	RefreshSelection(SelectionObject);
}

void FLrvModule::RefreshSelection(UObject* SelectionObject)
{
	static bool bIsReentrant = false;
	if (bIsReentrant) { return; }
	TGuardValue<bool> ReentrantGuard(bIsReentrant, true);
	if (!IsEnabled())
	{
		DestroyAllCreatedComponents();
		return;
	}
	if (!SelectionObject)
	{
		SelectionObject = GEditor->GetSelectedActors();
	}

	USelection* Selection = Cast<USelection>(SelectionObject);
	
	if (!Selection) { return; }
	TArray<AActor*> SelectedActors;
	Selection->GetSelectedObjects<AActor>(SelectedActors);
	CreateDebugComponentsForActors(SelectedActors);
	
	Selection->NoteSelectionChanged();
}

TSharedPtr<FLrvDebugVisualizer> FLrvModule::GetDefaultVisualizer()
{
	return StaticCastSharedPtr<FLrvDebugVisualizer>(
		GUnrealEd->FindComponentVisualizer(
			ULevelReferenceViewerComponent::StaticClass()
		)
	);
}

void FLrvModule::RegisterVisualizers()
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
	const auto* Settings = GetDefault<ULrvSettings>();
	auto ToRemove = RegisteredClasses;
	for (const auto Class : Settings->TargetSettings.TargetComponentClasses)
	{
		if (Class.IsPending())
		{
			ToRemove.Empty(); // do no removing if any class is pending
			UE_LOG(LogLrv, Log, TEXT("RegisterVisualizers: Skipping pending class"));
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
		const auto Visualizer = MakeShared<FLrvDebugVisualizer>();
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

void FLrvModule::UnregisterVisualizers()
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


void FLrvModule::Refresh(bool bForceRefresh)
{
	static bool bLastEnabled = !IsEnabled(); // force refresh on first call

	const auto bIsEnabled = IsEnabled();
	const auto bEnabledChanged = bLastEnabled != bIsEnabled;
	bLastEnabled = bIsEnabled;
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

void FLrvModule::ShutdownModule()
{
	if (!UObjectInitialized()) { return; }
	SettingsModifiedHandle.Reset();
	DestroyAllCreatedComponents();
	UnregisterVisualizers();
	FLrvCommands::Unregister();
	FLrvStyle::Shutdown();
}

void FLrvModule::AddTargetComponentClass(UClass* Class)
{
	auto* Settings = GetMutableDefault<ULrvSettings>();
	Settings->AddComponentClass(Class);
}

void FLrvModule::OnSettingsModified()
{
	Refresh(false);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLrvModule, CtrlLevelReferenceViewer)
