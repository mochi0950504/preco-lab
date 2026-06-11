// 《十樓的她》編輯器 Target 定義（在 UE 編輯器內開啟專案時使用）。
using UnrealBuildTool;
using System.Collections.Generic;

public class HotelVictoriaEditorTarget : TargetRules
{
	public HotelVictoriaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("HotelVictoria");
	}
}
