//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

using UnrealBuildTool;
using System.Collections.Generic;

public class SurvivalGameClientTarget : TargetRules
{
	public SurvivalGameClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;

		bUsesSteam = true;
		bUseLoggingInShipping = true;

		GlobalDefinitions.Add("UE4_PROJECT_STEAMGAMEDIR=\"Spacewar\"");
		GlobalDefinitions.Add("UE4_PROJECT_STEAMSHIPPINGID=480");

		ExtraModuleNames.AddRange( new string[] { "SurvivalGame" } );
	}
}
