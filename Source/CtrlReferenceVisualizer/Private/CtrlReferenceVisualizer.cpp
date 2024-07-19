#include "CtrlReferenceVisualizer.h"

#include "CrvCommands.h"
#include "CrvRefSearch.h"
#include "CrvSettings.h"
#include "CrvStyle.h"
#include "CrvUtils.h"
#include "ISettingsModule.h"
#include "LevelEditorSubsystem.h"
#include "ObjectEditorUtils.h"

#include "Styling/SlateIconFinder.h"

#include "UObject/Object.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"
using namespace CtrlRefViz;

DEFINE_LOG_CATEGORY(LogCrv);

void FCrvModule::MakeReferenceListSubMenu(UToolMenu* SubMenu, const ECrvDirection Direction) const
{
	if (!GetDefault<UCrvSettings>()->IsEnabled())
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
	auto CrvEditorSubsystem = GEditor->GetEditorSubsystem<UReferenceVisualizerEditorSubsystem>();
	auto MenuCache = CrvEditorSubsystem->MenuCache;
	MenuCache->FillCache(FCrvRefSearch::GetSelectionSet());

	bool bFoundEntry = false;
	FCrvSet Visited;
	auto ValidCache = MenuCache->GetValidCached(Direction);
	for (auto SelectedObject : FCrvRefSearch::GetSelectionSet())
	{
		if (!SelectedObject) { return; }
		UE_CLOG(IsDebugEnabled(), LogCrv, Log, TEXT("Find %s for SelectedObject: %s"), *SelectedObject->GetFullName(), Direction == ECrvDirection::Outgoing ? TEXT("Outgoing") : TEXT("Incoming"));
		auto Refs = MenuCache->GetReferences(SelectedObject, Direction);
		if (!Refs.Num()) { continue; }

		auto RefsArray = Refs.Array();
		RefsArray.Sort(
			[](const UObject& A, const UObject& B) { return A.GetFullName().Compare(B.GetFullName()) < 0; }
		);
		for (auto Ref : RefsArray)
		{
			if (Visited.Contains(Ref)) { continue; }

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
					FNewToolMenuDelegate::CreateRaw(this, &FCrvModule::MakeReferenceListSubMenu, ECrvDirection::Outgoing),
					FToolUIActionChoice(),
					EUserInterfaceActionType::None,
					false,
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlendSpaceEditor.ArrowRight")
				);
				const auto SubMenuIncoming = FToolMenuEntry::InitSubMenu(
					FName("CtrlReferenceVisualizerIncoming"),
					LOCTEXT("IncomingReferences", "Incoming"),
					LOCTEXT("IncomingReferencesTooltip", "List of incoming actor & component references to selected objects"),
					FNewToolMenuDelegate::CreateRaw(this, &FCrvModule::MakeReferenceListSubMenu, ECrvDirection::Incoming),
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
			return FCrvModule::IsEnabled();
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
						Settings->ToggleEnabled();
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
						return FCrvModule::IsEnabled();
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
						return GetDefault<UCrvSettings>()->bShowIncomingReferences;
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
						return GetDefault<UCrvSettings>()->bShowOutgoingReferences;
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
				[this](UToolMenu* SubMenu)
				{
					const auto MenuSection = SubMenu->AddSection("CtrlReferenceVisualizerSection", LOCTEXT("CtrlReferenceVisualizerSection", "Reference Visualizer"));
					SubMenu->AddMenuEntry(
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
										Settings->ToggleEnabled();
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
										return GetDefault<UCrvSettings>()->bIsEnabled;
									}
								)
							),
							EUserInterfaceActionType::ToggleButton
						)
					);

					SubMenu->AddMenuEntry(MenuSection.Name, FToolMenuEntry::InitSeparator(NAME_None));
					SubMenu->AddMenuEntry(MenuSection.Name, GetSettingsMenuEntry());
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

bool FCrvModule::IsEnabled()
{
	return GetDefault<UCrvSettings>()->IsEnabled();
}

bool FCrvModule::IsDebugEnabled()
{
	return IsEnabled() && GetDefault<UCrvSettings>()->bDebugEnabled;
}

void FCrvModule::ShutdownModule()
{
	if (!UObjectInitialized()) { return; }
	SettingsModifiedHandle.Reset();
	FCrvCommands::Unregister();
	FCrvStyle::Shutdown();
}

void FCrvModule::AddTargetComponentClass(UClass* Class)
{
	auto* Settings = GetMutableDefault<UCrvSettings>();
	Settings->AddComponentClass(Class);
}

void FCrvModule::InitCategories()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	auto IsRelevantClass = [](UClass* Class)
	{
		return Class->GetClassPathName().ToString().StartsWith("/Script/CtrlReferenceVisualizer");
	};

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