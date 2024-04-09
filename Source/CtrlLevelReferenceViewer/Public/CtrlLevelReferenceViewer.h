#pragma once

#include "CoreMinimal.h"
#include "LevelReferenceViewerComponent.h"
#include "HAL/IConsoleManager.h"

#include "Modules/ModuleManager.h"

class UToolMenu;
class FLrvDebugVisualizer;
class ULrvSettings;

namespace LrvConsoleVars
{
	static int32 IsEnabled = 1;
	static FAutoConsoleVariableRef CVarLevelReferenceViewerEnabled(
		TEXT("ctrl.LevelReferenceViewer"),
		IsEnabled,
		TEXT("Enable Actor Reference Viewer Level Editor Display")
	);
}

DECLARE_LOG_CATEGORY_EXTERN(LogLrv, Log, All);


class FLrvModule : public IModuleInterface
{
public:
	bool bDidRegisterVisualizers = false;
	void MakeReferenceListSubMenu(UToolMenu* SubMenu, bool bFindOutRefs) const;
	void InitActorMenu() const;
	void InitLevelMenus() const;
	void OnPostEngineInit();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/** end IModuleInterface implementation */

	virtual void RegisterVisualizers();
	virtual void UnregisterVisualizers();
	void Refresh(bool bForceRefresh = true);

	static TSharedPtr<FLrvDebugVisualizer> GetDefaultVisualizer();
	virtual void AddTargetComponentClass(class UClass* Class);

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static FLrvModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FLrvModule>("CtrlLevelReferenceViewer");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("CtrlLevelReferenceViewer");
	}

protected:
	static bool IsEnabled();
	void OnSelectionChanged(UObject* Object);
	void RefreshSelection(UObject* SelectionObject);
	void OnSettingsModified();
	void CreateDebugComponentsForActors(TArray<AActor*> Actors);
	void DestroyStaleDebugComponents(const TArray<AActor*>& CurrentActorSelection);
	void DestroyAllCreatedComponents();
	TArray<FName> RegisteredClasses;
	FDelegateHandle SettingsModifiedHandle;
	TArray<TWeakObjectPtr<ULevelReferenceViewerComponent>> CreatedDebugComponents;
};