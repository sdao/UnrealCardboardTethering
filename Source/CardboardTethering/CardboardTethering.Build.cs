// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules {
    public class CardboardTethering : ModuleRules {
        public CardboardTethering(TargetInfo Target) {
            string enginePath = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);

            PrivateIncludePaths.AddRange(
                new string[] {
                    "CardboardTethering/Private",
                    enginePath + "Source/Runtime/Renderer/Private",
                    // ... add other private include paths required here ...
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "RHI",
                    "RenderCore",
                    "Renderer",
                    "ShaderCore",
                    "HeadMountedDisplay",
                    "Projects",
                    "Slate",
                    "SlateCore",
                }
            );

            if (UEBuildConfiguration.bBuildEditor == true) {
                PrivateDependencyModuleNames.Add("UnrealEd");
            }

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "turbojpeg",
                    "libusb"
                    // ... add other public dependencies that you statically link with here ...
                }
            );

            AddEngineThirdPartyPrivateStaticDependencies(Target,
                "turbojpeg",
                "libusb"
                // ... add any third party modules that your module depends on here ...
            );
        }
    }
}
