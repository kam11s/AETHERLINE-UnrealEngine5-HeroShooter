using UnrealBuildTool;

public class AetherlineEditorTarget : TargetRules
{
	public AetherlineEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Aetherline");
	}
}
