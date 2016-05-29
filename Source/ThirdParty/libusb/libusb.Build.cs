// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class libusb : ModuleRules
{
	public libusb(TargetInfo Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "x64", "Release"));
			PublicAdditionalLibraries.Add("libusb-1.0.lib");

			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("libusb-1.0.dll");

            CopyToBinaries(Path.Combine(ModuleDirectory, "x64", "Release", "libusb-1.0.dll"), Target);
        }
	}

    private void CopyToBinaries(string Filepath, TargetInfo Target) {
        string binariesDir = Path.Combine(ModuleDirectory, "..", "..", "..", "Binaries", "ThirdParty", "libusb", Target.Platform.ToString());
        string filename = Path.GetFileName(Filepath);

        if (!Directory.Exists(binariesDir))
            Directory.CreateDirectory(binariesDir);

        if (!File.Exists(Path.Combine(binariesDir, filename)))
            File.Copy(Filepath, Path.Combine(binariesDir, filename), true);
    }
}
