// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Gunfight : ModuleRules
{
	public Gunfight(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //MinFilesUsingPrecompiledHeaderOverride = 1;

        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"InputCore", 
				"EnhancedInput", 
				"SlateCore",
				"HeadMountedDisplay", 
				"UMG",
                "OnlineSubsystem",
                "OnlineSubsystemEOS",
                "OnlineSubsystemUtils",
                "OVRPlatform",
            });

		PrivateDependencyModuleNames.AddRange(
			new string[] {
                "XRBase",
                "Json",
                "JsonUtilities",
                "EnhancedInput",
                "OnlineSubsystem",
                "OnlineSubsystemEOS",
                "OnlineSubsystemUtils",
            });

        if (Target.Platform == UnrealTargetPlatform.Android && Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            var manifestFile = Path.Combine(ModuleDirectory, "AndroidSanitizePermissions_UPL.xml");
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", manifestFile);
        }

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
