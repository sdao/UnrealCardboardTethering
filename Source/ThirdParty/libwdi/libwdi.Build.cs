// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class libwdi : ModuleRules
{
	public libwdi(TargetInfo Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "x64", "Release"));
			PublicAdditionalLibraries.Add("libwdi.lib");

			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("libwdi.dll");
			CopyToBinaries(Path.Combine(ModuleDirectory, "x64", "Release", "libwdi.dll"), Target);
    }
	}

    private void CopyToBinaries(string Filepath, TargetInfo Target) {
        string binariesDir = Path.Combine(ModuleDirectory, "..", "..", "..", "Binaries", "ThirdParty", "libwdi", Target.Platform.ToString());
        string filename = Path.GetFileName(Filepath);

        if (!Directory.Exists(binariesDir))
            Directory.CreateDirectory(binariesDir);

        if (!File.Exists(Path.Combine(binariesDir, filename)))
            File.Copy(Filepath, Path.Combine(binariesDir, filename), true);
    }
}
