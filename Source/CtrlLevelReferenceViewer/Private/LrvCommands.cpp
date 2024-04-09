#include "LrvCommands.h"

#include "LrvSettings.h"

#define LOCTEXT_NAMESPACE "LevelReferenceViewer"

void FLrvCommands::ToggleEnabled_Execute()
{
	auto const Settings = GetMutableDefault<ULrvSettings>();
	Settings->ToggleEnabled();
}

bool FLrvCommands::ToggleEnabled_CanExecute()
{
	return true;
}

bool FLrvCommands::ToggleEnabled_IsChecked()
{
	const auto Settings = GetDefault<ULrvSettings>();
	return Settings->bIsEnabled;
}

void FLrvCommands::RegisterCommands()
{
	UI_COMMAND(
		ToggleReferenceViewer,
		"Actor Reference Viewer",
		"Toggle Actor Reference Viewer",
		EUserInterfaceActionType::ToggleButton,
		FInputChord()
	);
	
	if (CommandList.IsValid())
	{
		return;
	}

	CommandList = MakeShareable(new FUICommandList);
	CommandList->MapAction(
		ToggleReferenceViewer,
		FExecuteAction::CreateRaw(this, &FLrvCommands::ToggleEnabled_Execute),
		FCanExecuteAction::CreateRaw(this, &FLrvCommands::ToggleEnabled_CanExecute),
		FIsActionChecked::CreateRaw(this, &FLrvCommands::ToggleEnabled_IsChecked)
	);
}

#undef LOCTEXT_NAMESPACE
