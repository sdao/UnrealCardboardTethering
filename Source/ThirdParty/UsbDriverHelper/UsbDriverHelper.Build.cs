// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class UsbDriverHelper : ModuleRules
{
	public UsbDriverHelper(TargetInfo Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			CopyToBinaries(Path.Combine(ModuleDirectory, "x64", "Release", "UsbDriverHelper.exe"), Target);
			CopyToBinaries(Path.Combine(ModuleDirectory, "..", "libwdi", "x64", "Release", "libwdi.dll"), Target);
    }
	}

    private void CopyToBinaries(string Filepath, TargetInfo Target) {
        string binariesDir = Path.Combine(ModuleDirectory, "..", "..", "..", "Binaries", "ThirdParty", "UsbDriverHelper", Target.Platform.ToString());
        string filename = Path.GetFileName(Filepath);

        if (!Directory.Exists(binariesDir))
            Directory.CreateDirectory(binariesDir);

        if (!File.Exists(Path.Combine(binariesDir, filename)))
            File.Copy(Filepath, Path.Combine(binariesDir, filename), true);
    }
}
