// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GunfightEditorTarget : TargetRules
{
	public GunfightEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
        BuildEnvironment = TargetBuildEnvironment.Unique;
        DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("Gunfight");
	}
}
