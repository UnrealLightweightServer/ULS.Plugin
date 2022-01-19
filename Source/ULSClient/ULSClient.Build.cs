// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ULSClient : ModuleRules
{
	public ULSClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
				new string[] {
					"ULSClient/Private",
					// ... add other private include paths required here ...
				});

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Sockets",
				"NetCore",
				"Networking",
				"WebSockets",
				"VaRest"
			}
		);
	}
}
