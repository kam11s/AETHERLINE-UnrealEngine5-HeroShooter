using UnrealBuildTool;

public class Aetherline : ModuleRules
{
	public Aetherline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"GameplayAbilities", "GameplayTags", "GameplayTasks", "UMG",
			"AIModule", "HTTP", "Json", "JsonUtilities"
		});
	}
}
