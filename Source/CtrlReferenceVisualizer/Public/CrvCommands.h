#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FCrvCommands : public TCommands<FCrvCommands>
{
public:
	TSharedPtr<FUICommandInfo> ToggleReferenceVisualizer;
	FCrvCommands()
		: TCommands<FCrvCommands>(
			TEXT("FCrvCommands"),
			NSLOCTEXT("ReferenceVisualizer", "ReferenceVisualizerCommands", "Reference Visualizer Commands"),
			NAME_None,
			FAppStyle::GetAppStyleSetName()
		)
	{
	}

	virtual ~FCrvCommands() override = default;

	void ToggleEnabled_Execute();
	bool ToggleEnabled_CanExecute();
	bool ToggleEnabled_IsChecked();
	virtual void RegisterCommands() override;
	
	TSharedPtr<FUICommandList> CommandList;
};