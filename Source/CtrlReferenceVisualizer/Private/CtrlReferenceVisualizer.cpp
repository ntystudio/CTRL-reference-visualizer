#include "CtrlReferenceVisualizer.h"

#include "CrvCommands.h"
#include "CrvDebugVisualizer.h"
#include "CrvRefSearch.h"
#include "CrvSettings.h"
#include "CrvStyle.h"
#include "CrvUtils.h"
#include "ISettingsModule.h"
#include "LevelEditorSubsystem.h"
#include "ObjectEditorUtils.h"
#include "ReferenceVisualizerComponent.h"
#include "Selection.h"
#include "UnrealEdGlobals.h"

#include "Editor/UnrealEdEngine.h"

#include "Styling/SlateIconFinder.h"

#include "UObject/Object.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"
using namespace CRV;

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

	MenuCache.FillCache(FCrvRefSearch::GetSelectionSet(), bFindOutRefs);

	bool bFoundEntry = false;
	FCrvSet Visited;
	auto ValidCache = MenuCache.GetValidCached(bFindOutRefs);
	for (auto SelectedObject : FCrvRefSearch::GetSelectionSet())
	{
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("Find %s for SelectedObject: %s"), *SelectedObject->GetFullName(), bFindOutRefs ? TEXT("Outgoing") : TEXT("Incoming"));
		auto FoundRefs = ValidCache.Find(SelectedObject);
		if (!FoundRefs) { continue; }
		auto Refs = *FoundRefs;

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
				// get human-readable name from uobject
				SectionPtr->Label = FText::FromString(BP->GetFriendlyName());
			}
			else
			{
				SectionPtr->Label = FText::FromString(Ref->GetClass()->GetName());
			}
			auto [Name, Label, ToolTip, Icon, Action] = FCrvRefSearch::MakeMenuEntry(SelectedObject, Ref);
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

void FCrvModule::SelectReference(UObject* Object)
{
	if (!Object) { return; }
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
	const bool bMoveCamera = GetDefault<UCrvSettings>()->bMoveViewportCameraToReference;
	if (const auto Actor = Cast<AActor>(Object))
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(Actor, true, true);
		if (bMoveCamera)
		{
			GEditor->MoveViewportCamerasToActor(*Actor, false);
		}
		return;
	}
	if (const auto Component = Cast<UActorComponent>(Object))
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectComponent(const_cast<UActorComponent*>(Component), true, true);
		if (bMoveCamera)
		{
			if (const auto SceneComponent = Cast<USceneComponent>(Component))
			{
				GEditor->MoveViewportCamerasToComponent(SceneComponent, false);
			}
			else
			{
				GEditor->MoveViewportCamerasToActor(*Component->GetOwner(), false);
			}
		}
		return;
	}
	
	if (const auto OwningComponent = Object->GetTypedOuter<USceneComponent>())
	{
		SelectReference(OwningComponent);
		return;
	}
	if (const auto OwningActor = Object->GetTypedOuter<AActor>())
	{
		SelectReference(OwningActor);
		return;
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

				const auto SubMenuOptions = FToolMenuEntry::InitSubMenu(
					FName("CtrlReferenceVisualizerOptions"),
					LOCTEXT("Options", "Options"),
					LOCTEXT("OptionsTooltip", "More Options"),
					FNewToolMenuDelegate::CreateRaw(this, &FCrvModule::MakeActorOptionsSubmenu),
					FToolUIActionChoice(),
					EUserInterfaceActionType::None,
					false,
					FSlateIcon()
				);
				Menu->AddMenuEntry(SubMenuOptions.Name, SubMenuOptions);
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


void FCrvModule::MakeActorOptionsSubmenu(UToolMenu* Menu) const
{
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
						const FToolMenuContext MenuContext;
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
					const auto MenuSection = Menu->AddSection("CtrlReferenceVisualizerSection", LOCTEXT("CtrlReferenceVisualizerSection", "Reference Visualizer"));
					Menu->AddMenuEntry(
						MenuSection.Name,
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
					Menu->AddMenuEntry(MenuSection.Name, FToolMenuEntry::InitSeparator(NAME_None));
					Menu->AddMenuEntry(MenuSection.Name, GetSettingsMenuEntry());
				}
			)
		)
	);
	ReferenceVisualizerSubMenu.Icon = FSlateIconFinder::FindIcon("BTEditor.Graph.BTNode.Task.RunBehavior.Icon");
	InNewToolMenu->AddMenuEntry("CtrlEditorSubmenu", ReferenceVisualizerSubMenu);
}

class SCrvDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCrvDialog) {}
	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	// virtual ~SCrvDialog() override;
};

void SCrvDialog::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
		.Padding(FMargin(0.0f, 3.0f, 1.0f, 0.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CtrlReferenceVisualizer", "Content"))
			]
		]
	];
	
}

void FCrvModule::InitTab()
{
	// create and register tab
	const FText TitleText = LOCTEXT("CrvWindowTitle", "Ctrl Reference Visualizer");
	// Create the window to pick the class
	TSharedRef<SWindow>  CrvWindow = SNew(SWindow)
		.Title(TitleText)
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(1000.f, 700.f))
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false);
	 
	TSharedRef<SCrvDialog> Dialog = SNew(SCrvDialog);

	CrvWindow->SetContent(Dialog);
	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	if (RootWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(CrvWindow, RootWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(CrvWindow);
	}

	
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
	InitCategories();
	UCrvSettings* Settings = GetMutableDefault<UCrvSettings>();
	SettingsModifiedHandle = Settings->OnModified.AddRaw(this, &FCrvModule::OnSettingsModified);
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
	UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("DestroyAllCreatedComponents"));
	for (auto ExistingComponent : CreatedDebugComponents)
	{
		if (!ExistingComponent.IsValid())
		{
			continue;
		}
		if (auto OwningActor = ExistingComponent->GetOwner())
		{
			UE_CLOG(IsDebugEnabled(), LogCrv, Warning, TEXT("DestroyAllCreatedComponents: Remove %s from %s"), *ExistingComponent->GetName(), *GetDebugName(OwningActor));
			OwningActor->RemoveOwnedComponent(ExistingComponent.Get());
		}
		else
		{
			UE_CLOG(IsDebugEnabled(), LogCrv, Warning, TEXT("DestroyAllCreatedComponents: Actor is not valid"));
		}
		ExistingComponent->DestroyComponent();
	}

	CreatedDebugComponents.Empty();
}

void FCrvModule::DestroyStaleDebugComponents(const TArray<AActor*>& CurrentActorSelection)
{
	// destroy any created debug components that are no longer needed
	auto CurrentlyCreatedDebugComponents = CreatedDebugComponents;
	for (auto ExistingComponent : CurrentlyCreatedDebugComponents)
	{
		if (!ExistingComponent.IsValid())
		{
			// if the component is no longer valid, remove it from the list
			CreatedDebugComponents.Remove(ExistingComponent);
			continue;
		}

		if (auto OwningActor = ExistingComponent->GetOwner())
		{
			// if the actor is no longer selected, destroy the component
			if (!CurrentActorSelection.Contains(OwningActor))
			{
				UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("DestroyStaleDebugComponents: Destroying debug component for actor %s"), *GetDebugName(OwningActor));
				CreatedDebugComponents.Remove(ExistingComponent);
				ExistingComponent->DestroyComponent();
			}
			// otherwise, keep the component
		}
		else
		{
			UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("DestroyStaleDebugComponents: Destroying debug component for invalid actor."));
			// if the actor is no longer valid, destroy the component
			CreatedDebugComponents.Remove(ExistingComponent);
			ExistingComponent->DestroyComponent();
		}
	}
	if (CurrentlyCreatedDebugComponents.Num() != CreatedDebugComponents.Num())
	{
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("DestroyStaleDebugComponents: Destroyed %d stale debug components"), CurrentlyCreatedDebugComponents.Num() - CreatedDebugComponents.Num());
	}
}

bool FCrvModule::HasDebugComponentForActor(const AActor* Actor) const
{
	if (Actor->FindComponentByClass<UReferenceVisualizerComponent>())
	{
		return true;
	}
	for (const auto ExistingComponent : CreatedDebugComponents)
	{
		if (!ExistingComponent.IsValid()) { UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("HasDebugComponentForActor: Found invalid debug component")); continue; }
		if (ExistingComponent->GetOwner() == Actor)
		{
			UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("HasDebugComponentForActor: Found debug component for actor %s that didn't appear in FindComponent"), *GetDebugName(Actor));
			return true;
		}
	}
	return false;
}

bool FCrvModule::CreateDebugComponentsForActors(TArray<AActor*> Actors)
{
	DestroyStaleDebugComponents(Actors);
	TArray<UReferenceVisualizerComponent*> NewDebugComponents;
	NewDebugComponents.Reserve(Actors.Num());
	for (const auto Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			UE_CLOG(IsDebugEnabled(), LogCrv, Warning, TEXT("CreateDebugComponentsForActors: Actor is not valid"));
			continue;
		}
		
		// don't add debug components to actors that already have them (either auto-created or manually added)
		if (HasDebugComponentForActor(Actor))
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
			UE_CLOG(IsDebugEnabled(), LogCrv, Warning, TEXT("CreateDebugComponentsForActors: Failed to add component to actor"));
			continue;
		}
		
		Actor->AddOwnedComponent(DebugComponent);
		if (!DebugComponent->IsRegistered())
		{
			DebugComponent->RegisterComponent();
		}
		
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("CreateDebugComponentsForActors: Created debug component for actor %s"), *Actor->GetName());
		NewDebugComponents.Add(DebugComponent);
	}
	UE_CLOG(
		IsDebugEnabled(),
		LogCrv,
		Log,
		TEXT("CreateDebugComponentsForActors: Created %d debug components for %d actors. Total created components: %d"),
		NewDebugComponents.Num(),
		Actors.Num(),
		CreatedDebugComponents.Num()
	);
	CreatedDebugComponents.Append(NewDebugComponents);
	return NewDebugComponents.Num() > 0;
}

bool FCrvModule::IsEnabled()
{
	const auto Settings = GetDefault<UCrvSettings>();
	return Settings->bIsEnabled;
}

bool FCrvModule::IsDebugEnabled()
{
	const auto Settings = GetDefault<UCrvSettings>();
	return Settings->bDebugEnabled;
}

void FCrvModule::OnSelectionChanged(UObject* SelectionObject)
{
	if (bIsRefreshingSelection) { return; }
	TGuardValue<bool> ReentrantGuard(bIsRefreshingSelection, true);
	if (!IsEnabled()) { DestroyAllCreatedComponents(); return; }
	// if (SelectionObject != GEditor->GetSelectedActors()) { return; }
	UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("OnSelectionChanged"));
	// RegisterVisualizers();
	RefreshSelection();
}

void FCrvModule::RefreshSelection()
{
	if (!IsEnabled())
	{
		DestroyAllCreatedComponents();
		return;
	}

	USelection* Selection = Cast<USelection>(GEditor->GetSelectedActors());
	if (!Selection)
	{
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("RefreshSelection: No selection"));
		return;
	}

	TArray<AActor*> SelectedActors;
	Selection->GetSelectedObjects<AActor>(SelectedActors);
	UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("RefreshSelection: %d actors selected"), SelectedActors.Num());
	if (CreateDebugComponentsForActors(SelectedActors))
	{
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("RefreshSelection: Created debug components for %d actors"), SelectedActors.Num());
		Selection->BeginBatchSelectOperation();
		Selection->ForceBatchDirty();
		for (const auto SelectedActor : SelectedActors)
		{
			Selection->Deselect(SelectedActor);
			Selection->Select(SelectedActor);
		}
		Selection->EndBatchSelectOperation();
		Selection->NoteSelectionChanged();
	}
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
	UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("RegisterVisualizers"));

	// track items to remove.
	// if item is in RegisteredClasses but not in Classes, then it should be removed
	// start with all registered classes, remove as we find them in Classes
	// at the end, all items left in ToRemove should be removed
	const auto* Settings = GetDefault<UCrvSettings>();
	auto ToRemove = RegisteredClasses; // all registered classes
	for (const auto SoftClassPtr : Settings->TargetSettings.TargetComponentClasses)
	{
		if (SoftClassPtr.IsPending())
		{
			ToRemove.Empty(); // do no removing if any class is pending
			UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("\tRegisterVisualizers: Skipping pending class %s"), *SoftClassPtr.ToString());
			continue;
		}
		if (SoftClassPtr.IsNull()) { continue; } // do nothing if class is null
		if (!SoftClassPtr.IsValid()) { continue; } // invalid & not pending class will be removed

		auto ClassName = SoftClassPtr->GetFName();
		ToRemove.Remove(ClassName); // remove from removal list (do first, so we don't accidentally remove the class)
		// do nothing if class is already registered
		if (GUnrealEd->FindComponentVisualizer(ClassName)) { continue; }

		// don't register classes that are already registered
		if (RegisteredClasses.Contains(ClassName)) { continue; }
		RegisteredClasses.Add(ClassName);
		const auto Visualizer = MakeShared<FCrvDebugVisualizer>();
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("\tRegister Visualizer: %s"), *ClassName.ToString());
		GUnrealEd->RegisterComponentVisualizer(ClassName, Visualizer);
		Visualizer->OnRegister();
	}

	// remove any previously registered classes that are no longer registered
	for (const auto ClassName : ToRemove)
	{
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("\tUnregister Visualizer: %s"), *ClassName.ToString());
		RegisteredClasses.Remove(ClassName);
		GUnrealEd->UnregisterComponentVisualizer(ClassName);
	}
	bDidRegisterVisualizers = true;
}

void FCrvModule::UnregisterVisualizers()
{
	auto CurrentClasses = RegisteredClasses;
	RegisteredClasses.Empty();
	bDidRegisterVisualizers = false;
	if (!GUnrealEd) { return; }
	UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("UnregisterVisualizers"));
	for (const auto ClassName : CurrentClasses)
	{
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("\tUnregister Visualizer: %s"), *ClassName.ToString());
		GUnrealEd->UnregisterComponentVisualizer(ClassName);
	}
}


void FCrvModule::Refresh(const bool bForceRefresh)
{
	if (!GetDefault<UCrvSettings>()->bRefreshEnabled) { return; }

	static bool bLastEnabled = !IsEnabled(); // force refresh on first call
	const auto bIsEnabled = IsEnabled();
	const auto bEnabledChanged = bLastEnabled != bIsEnabled;
	bLastEnabled = bIsEnabled;
	UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("Refresh: bForceRefresh: %d, bIsEnabled: %d, bEnabledChanged: %d"), bForceRefresh, bIsEnabled, bEnabledChanged);
	if (bForceRefresh)
	{
		UnregisterVisualizers();
	}

	bIsEnabled ? RegisterVisualizers() : UnregisterVisualizers();

	if (bEnabledChanged || bForceRefresh)
	{
		RefreshSelection();
	}
}
void FCrvModule::QueueRefresh(bool bForceRefresh)
{
	if (!IsEnabled()) { return; }
	if (!GEditor) { return; }
	if (GEditor->GetTimerManager()->IsTimerPending(RefreshTimerHandle)) { return; } // already queued
	RefreshTimerHandle = GEditor->GetTimerManager()->SetTimerForNextTick(
		FTimerDelegate::CreateRaw(this, &FCrvModule::Refresh, bForceRefresh)
	);
}

void FCrvModule::ShutdownModule()
{
	if (!UObjectInitialized()) { return; }
	SettingsModifiedHandle.Reset();
	RefreshTimerHandle.Invalidate();
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

void FCrvModule::OnSettingsModified(UObject* Object, FProperty* Property)
{
	Refresh(false);
}

void FCrvModule::InitCategories()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	auto IsRelevantClass = [](UClass* Class) { return Class->GetClassPathName().ToString().StartsWith("/CtrlReferenceVisualizer"); };

	for (auto* const Class : TObjectRange<UClass>())
	{
		if (!IsRelevantClass(Class)) { continue; }
		const TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection(
			Class->GetFName(),
			TEXT("CtrlReferenceVisualizer"),
			LOCTEXT("CtrlReferenceVisualizer", "💛 CTRL Reference Visualizer")
		);

		// Find all categories for this class
		for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			const auto* const Property = *It;
			if (!IsRelevantClass(Property->GetOwnerClass())) { continue; }

			auto Category = FObjectEditorUtils::GetCategoryFName(Property).ToString();

			// prevent crash
			if (Category.Contains(TEXT("|")))
			{
				Category = Category.Left(Category.Find(TEXT("|")));
			}
			Section->AddCategory(FName(*Category.TrimStartAndEnd()));
		}
		Section->AddCategory(TEXT("Default"));
		Section->AddCategory(Class->GetFName());
	}
}
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCrvModule, CtrlReferenceVisualizer)