using UnrealBuildTool;
public class AetherlineEditorTarget : TargetRules
{
	public AetherlineEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		ExtraModuleNames.Add("Aetherline");
	}
}
