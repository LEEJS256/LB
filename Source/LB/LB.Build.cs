// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LB : ModuleRules
{
	public LB(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			//MortionWarp
			"MotionWarping",
			// AI
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			//카메라 쉐이크
			"GameplayCameras",
			"EngineCameras",
			// Niagara 관련 추가
			"Niagara","NiagaraCore", "NiagaraShader", "RenderCore",
			// AI
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			// GAS
			"GameplayStateTreeModule",
			"GameplayAbilities",       
			"GameplayTags",            
			"GameplayTasks",   
			// UI
			"UMG", 
			"Slate",
			"SlateCore",
			"CommonUI", 
			"CommonInput",
			"Paper2D",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"LB",
			"LB/Variant_Strategy",
			"LB/Variant_Strategy/UI",
			"LB/Variant_TwinStick",
			"LB/Variant_TwinStick/AI",
			"LB/Variant_TwinStick/Gameplay",
			"LB/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
