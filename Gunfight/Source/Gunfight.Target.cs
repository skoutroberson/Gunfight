// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GunfightTarget : TargetRules
{
	public GunfightTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        BuildEnvironment = TargetBuildEnvironment.Unique;
        bUseLoggingInShipping = true;
        DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("Gunfight");
	}
}
