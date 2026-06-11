// 《十樓的她》遊戲執行檔 Target 定義（Game / 出貨版皆由此建置）。
using UnrealBuildTool;
using System.Collections.Generic;

public class HotelVictoriaTarget : TargetRules
{
	public HotelVictoriaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("HotelVictoria");
	}
}
