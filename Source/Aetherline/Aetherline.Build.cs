using UnrealBuildTool;

public class Aetherline : ModuleRules
{
	public Aetherline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"UMG", "Slate", "SlateCore", "AIModule", "NavigationSystem",
			"NetCore", "OnlineSubsystem", "OnlineSubsystemUtils"
		});
	}
}
