using UnrealBuildTool;

public class AetherlineTarget : TargetRules
{
	public AetherlineTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		ExtraModuleNames.Add("Aetherline");
	}
}
