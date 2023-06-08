// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StealthGameEditor : ModuleRules
{
	public StealthGameEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
            new string[]
            {
                "StealthGameEditor/Public"
            });

        PrivateIncludePaths.AddRange(
            new string[] 
            {
        		"StealthGameEditor/Private"
        	});

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "Engine", "CoreUObject", "StealthGame" });
		
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd"});
	}
}
