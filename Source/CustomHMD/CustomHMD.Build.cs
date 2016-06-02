// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class CustomHMD : ModuleRules
	{
		public CustomHMD(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"CustomHMD/Private",
					Path.GetDirectoryName(RulesCompiler.GetModuleFilename("Renderer"))
					    + "/Private",
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
                    "Projects"
				}
				);

            PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "turbojpeg",
                "libusb"
				// ... add other public dependencies that you statically link with here ...
			}
            );

            AddThirdPartyPrivateStaticDependencies(Target,
                "turbojpeg",
                "libusb"
                // ... add any third party modules that your module depends on here ...
                );
        }
	}
}
