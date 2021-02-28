//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class SurvivalGameServerTarget : TargetRules
{
	public SurvivalGameServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;

		bUsesSteam = true;
		bUseLoggingInShipping = true;

		GlobalDefinitions.Add("UE4_PROJECT_STEAMPRODUCTNAME=\"Spacewar\"");
		GlobalDefinitions.Add("UE4_PROJECT_STEAMGAMEDESC=\"SurvivalGame\"");
		GlobalDefinitions.Add("UE4_PROJECT_STEAMGAMEDIR=\"Spacewar\"");
		GlobalDefinitions.Add("UE4_PROJECT_STEAMSHIPPINGID=480");

		ExtraModuleNames.AddRange( new string[] { "SurvivalGame" } );
	}
}
