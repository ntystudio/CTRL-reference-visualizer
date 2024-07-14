#include "CrvCommands.h"

#include "CrvSettings.h"

#define LOCTEXT_NAMESPACE "ReferenceVisualizer"

void FCrvCommands::ToggleEnabled_Execute()
{
	const auto Settings = GetMutableDefault<UCrvSettings>();
	Settings->ToggleEnabled();
}

bool FCrvCommands::ToggleEnabled_CanExecute()
{
	return true;
}

bool FCrvCommands::ToggleEnabled_IsChecked()
{
	const auto Settings = GetDefault<UCrvSettings>();
	return Settings->bIsEnabled;
}

void FCrvCommands::RegisterCommands()
{
	UI_COMMAND(
		ToggleReferenceVisualizer,
		"Reference Visualizer",
		"Toggle Reference Visualizer",
		EUserInterfaceActionType::ToggleButton,
		FInputChord()
	);
	
	if (CommandList.IsValid())
	{
		return;
	}

	CommandList = MakeShareable(new FUICommandList);
	CommandList->MapAction(
		ToggleReferenceVisualizer,
		FExecuteAction::CreateRaw(this, &FCrvCommands::ToggleEnabled_Execute),
		FCanExecuteAction::CreateRaw(this, &FCrvCommands::ToggleEnabled_CanExecute),
		FIsActionChecked::CreateRaw(this, &FCrvCommands::ToggleEnabled_IsChecked)
	);
}

#undef LOCTEXT_NAMESPACE
