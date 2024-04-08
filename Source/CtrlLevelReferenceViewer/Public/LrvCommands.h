#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FLrvCommands : public TCommands<FLrvCommands>
{
public:
	TSharedPtr<FUICommandInfo> ToggleReferenceViewer;
	FLrvCommands()
		: TCommands<FLrvCommands>(
			TEXT("FLrvCommands"),
			NSLOCTEXT("LevelReferenceViewer", "LevelReferenceViewerCommands", "Actor Reference Viewer Commands"),
			NAME_None,
			FAppStyle::GetAppStyleSetName()
		)
	{
	}

	virtual ~FLrvCommands() override = default;

	void ToggleEnabled_Execute();
	bool ToggleEnabled_CanExecute();
	bool ToggleEnabled_IsChecked();
	virtual void RegisterCommands() override;
	
	TSharedPtr<FUICommandList> CommandList;
};