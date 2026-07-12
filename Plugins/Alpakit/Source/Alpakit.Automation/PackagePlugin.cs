using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using AutomationTool;
using EpicGames.Core;
using UnrealBuildTool;
using AutomationScripts;

namespace Alpakit.Automation
{
	public class PackagePlugin : BuildCommand
	{
		private const string BaseReleaseVersion = "SN2Base";
		private const string WorkingBuildId = "SN2ML";
		private const string SteamAppId = "1962700";

		private static string GetGameLaunchURL()
		{
			return "steam://rungameid/" + SteamAppId;
		}

		private static string ReadGameInstallDirectory(FileReference projectFile)
		{
			var path = Path.Combine(projectFile.Directory.FullName, "GameInstallDirectory.txt");
			if (!FileExists(path))
			{
				throw new AutomationException("GameInstallDirectory.txt not found next to the .uproject at '{0}'", path);
			}

			foreach (var rawLine in File.ReadAllLines(path))
			{
				var line = rawLine.Trim();
				if (line.Length == 0 || line.StartsWith(";"))
				{
					continue;
				}
				return line;
			}

			throw new AutomationException("GameInstallDirectory.txt at '{0}' contains no path", path);
		}

		private static ProjectParams GetParams(BuildCommand cmd, FileReference projectFile, string pluginName)
		{
			var basedOnReleaseVersion = cmd.ParseOptionalStringParam("BasedOnReleaseVersion") ?? BaseReleaseVersion;

			var additionalCookerOptions =
				"-CookAgainstFixedBase -AllowUncookedAssetReferences " +
				"-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:GameDefaultMap=/Engine/Maps/Entry";

			var projectParameters = new ProjectParams(
				projectFile,
				Command: cmd,
				ClientTargetPlatforms: new List<TargetPlatformDescriptor>
				{
					new TargetPlatformDescriptor(UnrealTargetPlatform.Win64)
				},
				ClientConfigsToBuild: new List<UnrealTargetConfiguration>
				{
					UnrealTargetConfiguration.Shipping
				},

				Cook: true,
				AdditionalCookerOptions: additionalCookerOptions,
				DLCIncludeEngineContent: false,
				Compressed: true,
				Pak: true,
				Stage: true,
				DLCName: pluginName,

				BasedOnReleaseVersion: basedOnReleaseVersion
			);

			projectParameters.ValidateAndLog();
			return projectParameters;
		}

		private static void TryUpdateModulesFile(string filePath, string targetBuildId)
		{
			try
			{
				var modulesObject = JsonObject.Read(new FileReference(filePath));
				var modulesSubObject = modulesObject.GetObjectField("Modules");

				using (var writer = new JsonWriter(filePath))
				{
					writer.WriteObjectStart();
					writer.WriteValue("BuildId", targetBuildId);

					writer.WriteObjectStart("Modules");
					foreach (var moduleName in modulesSubObject.KeyNames)
					{
						var modulePath = modulesSubObject.GetStringField(moduleName);
						writer.WriteValue(moduleName, modulePath);
					}

					writer.WriteObjectEnd();
					writer.WriteObjectEnd();
				}
			}
			catch (Exception ex)
			{
				throw new AutomationException("Failed to update modules file '{0}': {1}", filePath, ex.Message);
			}
		}

		private static void UpdateModulesBuildId(string stagingDir, string targetBuildId)
		{
			foreach (var modulesFile in FindFiles("*.modules", true, stagingDir))
			{
				TryUpdateModulesFile(modulesFile, targetBuildId);
			}
		}

		private static IReadOnlyList<DeploymentContext> CreateDeploymentContexts(ProjectParams projectParams)
		{
			var deployContextList = new List<DeploymentContext>();
			if (!projectParams.NoClient)
			{
				deployContextList.AddRange(Project.CreateDeploymentContext(projectParams, false));
			}
			return deployContextList;
		}

		private static void RemapCookedPluginsContentPaths(ProjectParams projectParams, IEnumerable<DeploymentContext> deploymentContexts)
		{
			foreach (var deploymentContext in deploymentContexts)
			{
				var projectName = projectParams.RawProjectPath.GetFileNameWithoutAnyExtensions();

				string dlcSourceDirectory;
				if (projectParams.DLCFile.IsUnderDirectory(deploymentContext.EngineRoot))
					dlcSourceDirectory = Path.Combine("Engine", projectParams.DLCFile.Directory.ParentDirectory.MakeRelativeTo(deploymentContext.EngineRoot));
				else if (projectParams.DLCFile.IsUnderDirectory(deploymentContext.ProjectRoot))
					dlcSourceDirectory = Path.Combine(projectName, projectParams.DLCFile.Directory.ParentDirectory.MakeRelativeTo(deploymentContext.ProjectRoot));
				else
					throw new Exception("Unknown DLC remap for DLC " + projectParams.DLCFile.GetFileNameWithoutExtension());

				var destinationModsDir = Path.Combine(projectName, "Mods");

				deploymentContext.RemapDirectories.Add(Tuple.Create(
					new StagedDirectoryReference(dlcSourceDirectory),
					new StagedDirectoryReference(destinationModsDir)));
			}
		}

		private static void CopyBuildToStagingDirectory(ProjectParams Params, IReadOnlyList<DeploymentContext> deploymentContexts)
		{
			var platformCares = Params.ClientTargetPlatformInstances[0].RequiresPak(Params);
			var requiresPak = platformCares == Platform.PakType.Always || (Params.Pak && platformCares != Platform.PakType.Never);

			if (requiresPak || (Params.Stage && !Params.SkipStage))
			{
				LogInformation("********** STAGE COMMAND STARTED **********");

				foreach (var sc in deploymentContexts)
				{
					Project.CreateStagingManifest(Params, sc);
					Project.CleanStagingDirectory(Params, sc);
				}
				foreach (var sc in deploymentContexts)
				{
					Project.ApplyStagingManifest(Params, sc);
				}

				LogInformation("********** STAGE COMMAND COMPLETED **********");
			}
		}

		private static void PackagePluginProject(IEnumerable<DeploymentContext> deploymentContexts, string workingBuildId)
		{
			foreach (var deploymentContext in deploymentContexts)
			{
				UpdateModulesBuildId(deploymentContext.StageDirectory.ToString(), workingBuildId);
			}
		}

		private static void InstallPluginToGame(ProjectParams projectParams,
			IEnumerable<DeploymentContext> deploymentContexts, string gameInstallDir)
		{
			var projectName = projectParams.RawProjectPath.GetFileNameWithoutAnyExtensions();
			var projectRoot = projectParams.RawProjectPath.Directory;
			var projectPluginsFolder = DirectoryReference.Combine(projectRoot, "Plugins");
			var pluginRelToProject = projectParams.DLCFile.Directory.MakeRelativeTo(projectRoot);
			var pluginRelToPlugins = projectParams.DLCFile.Directory.MakeRelativeTo(projectPluginsFolder);

			foreach (var sc in deploymentContexts)
			{
				if (sc.FinalCookPlatform != "Windows")
				{
					continue;
				}

				var stageRoot = sc.StageDirectory.ToString();
				var modsStaged = Path.Combine(stageRoot, projectName, "Mods", pluginRelToPlugins);
				var pluginsStaged = Path.Combine(stageRoot, projectName, pluginRelToProject);

				if (DirectoryExists(pluginsStaged))
				{
					MergeDirectory_NoExceptions(pluginsStaged, modsStaged);
				}

				var dest = Path.Combine(gameInstallDir, "Mods", pluginRelToPlugins);
				if (DirectoryExists(dest))
				{
					DeleteDirectory(dest);
				}
				CreateDirectory(dest);
				CopyDirectory_NoExceptions(modsStaged, dest);

				LogInformation("Installed plugin '{0}' to '{1}'", pluginRelToPlugins, dest);
			}
		}

		private static void CleanStagingDirectories(IEnumerable<DeploymentContext> deploymentContexts)
		{
			foreach (var deploymentContext in deploymentContexts)
			{
				if (DirectoryExists(deploymentContext.StageDirectory.ToString()))
				{
					DeleteDirectory(deploymentContext.StageDirectory);
				}
			}
		}

		public override void ExecuteBuild()
		{
			var projectFile = new FileReference(ParseRequiredStringParam("Project"));
			var pluginName = ParseRequiredStringParam("PluginName");
			var bLaunchGame = ParseParam("LaunchGame");

			var gameInstallDir = ParseOptionalStringParam("GameDir");
			if (string.IsNullOrEmpty(gameInstallDir))
			{
				gameInstallDir = ReadGameInstallDirectory(projectFile);
			}
			if (!DirectoryExists(gameInstallDir))
			{
				throw new AutomationException("Game install directory does not exist: '{0}'", gameInstallDir);
			}

			var projectParams = GetParams(this, projectFile, pluginName);

			Project.Cook(projectParams);
			var deploymentContexts = CreateDeploymentContexts(projectParams);
			RemapCookedPluginsContentPaths(projectParams, deploymentContexts);

			try
			{
				CopyBuildToStagingDirectory(projectParams, deploymentContexts);
				PackagePluginProject(deploymentContexts, WorkingBuildId);
				InstallPluginToGame(projectParams, deploymentContexts, gameInstallDir);
			}
			finally
			{
				CleanStagingDirectories(deploymentContexts);
			}

			if (bLaunchGame)
			{
				Process.Start(new ProcessStartInfo(GetGameLaunchURL()) { UseShellExecute = true });
			}
		}
	}
}
