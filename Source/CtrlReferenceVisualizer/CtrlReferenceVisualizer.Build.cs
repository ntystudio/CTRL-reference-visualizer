using UnrealBuildTool;

public class CtrlReferenceVisualizer : ModuleRules
{
	public CtrlReferenceVisualizer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
#if UE_5_2_OR_LATER
		IWYUSupport = IWYUSupport.Full;
#else
		bEnforceIWYU = true;
#endif
		CppStandard = CppStandardVersion.Cpp20;

		PublicIncludePaths.AddRange(
			new string[]
			{
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
			}
		);

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"RenderCore",
				"RHI",
				"Slate",
				"SlateCore",
				"UMG",
				"Paper2D"
			}
		);
		
		// PublicDependencyModuleNames.AddRange(
		// 	new string[] {
		// 		"Core",
		// 		"CoreUObject",
		// 		"Engine",
		// 		"RHI",
  //       		        "SlateCore",
  //       		        "Slate",
  //               		"NavigationSystem"
		// 	}
		// );
		//
		// PrivateDependencyModuleNames.AddRange(
		// 	new string[] {
		// 	}
		// );
		//
		// if (Target.bBuildEditor == true)
		// {
		// 	//@TODO: Needed for the triangulation code used for sprites (but only in editor mode)
		// 	//@TOOD: Try to move the code dependent on the triangulation code to the editor-only module
		// 	PrivateDependencyModuleNames.Add("EditorFramework");
		// 	PrivateDependencyModuleNames.Add("UnrealEd");
		// }

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
				"Engine",
				"GameplayTags",
				"InputCore",
				"Json",
				"LevelEditor",
				"EditorSubsystem",
				"EditorFramework",
				"Renderer",
				"RenderCore",
				"TypedElementRuntime",
				"TypedElementFramework",
				"ToolMenus",
				"UnrealEd"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}