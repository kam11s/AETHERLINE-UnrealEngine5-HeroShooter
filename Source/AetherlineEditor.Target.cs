using UnrealBuildTool;

public class AetherlineEditorTarget : TargetRules
{
	public AetherlineEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		ExtraModuleNames.Add("Aetherline");
	}
}
