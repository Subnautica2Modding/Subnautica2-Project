#include "ModActions.h"
#include "SN2ModToolsSettings.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/DataAsset.h"
#include "Engine/PrimaryAssetLabel.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/DataAssetFactory.h"
#include "WidgetBlueprintFactory.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Async/Async.h"
#include "UObject/SavePackage.h"
#include "Editor.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"

DEFINE_LOG_CATEGORY_STATIC(LogSN2ModTools, Log, All);

static void ShowNotification(const FString& Message, bool bSuccess)
{
	FNotificationInfo Info(FText::FromString(Message));
	Info.ExpireDuration = 5.f;
	Info.bFireAndForget = true;
	Info.Image = bSuccess
		? FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"))
		: FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
	FSlateNotificationManager::Get().AddNotification(Info);
}

static int32 FindNextChunkId()
{
	TSet<int32> Used;

	IAssetRegistry& AR =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/Mods"));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UPrimaryAssetLabel::StaticClass()->GetClassPathName());

	TArray<FAssetData> Assets;
	AR.GetAssets(Filter, Assets);

	for (const FAssetData& Data : Assets)
	{
		UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Data.GetAsset());
		if (Label && Label->Rules.ChunkId > 0)
		{
			Used.Add(Label->Rules.ChunkId);
		}
	}

	for (int32 Id = 1; Id <= 300; ++Id)
	{
		if (!Used.Contains(Id)) return Id;
	}

	return -1;
}

bool ModActions::CreateMod(const FString& ModName)
{
	const FString PackageBase = FString::Printf(TEXT("/Game/Mods/%s"), *ModName);

	{
		IAssetRegistry& AR =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		TArray<FAssetData> Existing;
		FARFilter Filter;
		Filter.PackagePaths.Add(FName(*PackageBase));
		AR.GetAssets(Filter, Existing);
		if (Existing.Num() > 0)
		{
			ShowNotification(
				FString::Printf(TEXT("Mod '%s' already exists."), *ModName), false);
			return false;
		}
	}

	const int32 ChunkId = FindNextChunkId();
	if (ChunkId < 0)
	{
		ShowNotification(TEXT("No free ChunkID available in range 1-300."), false);
		return false;
	}

	FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	IAssetTools& AssetTools = AssetToolsModule.Get();

	bool bAllOk = true;

	{
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = AActor::StaticClass();
		UObject* Asset = AssetTools.CreateAsset(
			TEXT("ModActor"), PackageBase, UBlueprint::StaticClass(), Factory);
		if (!Asset)
		{
			UE_LOG(LogSN2ModTools, Warning, TEXT("Failed to create ModActor for %s"), *ModName);
			bAllOk = false;
		}
	}

	{
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		const FString WBPName = FString::Printf(TEXT("WBP_%s"), *ModName);
		UObject* Asset = AssetTools.CreateAsset(
			WBPName, PackageBase, Factory->GetSupportedClass(), Factory);
		if (!Asset)
		{
			UE_LOG(LogSN2ModTools, Warning, TEXT("Failed to create WBP_%s"), *ModName);
			bAllOk = false;
		}
	}

	{
		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = UPrimaryAssetLabel::StaticClass();
		const FString PALName = FString::Printf(TEXT("PAL_%s"), *ModName);
		UObject* Asset = AssetTools.CreateAsset(
			PALName, PackageBase, UPrimaryAssetLabel::StaticClass(), Factory);
		if (UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Asset))
		{
			Label->Rules.ChunkId = ChunkId;
			Label->Rules.CookRule = EPrimaryAssetCookRule::AlwaysCook;
			Label->bLabelAssetsInMyDirectory = true;
			Label->MarkPackageDirty();

			UPackage* Pkg = Label->GetOutermost();
			const FString DiskPath = FPackageName::LongPackageNameToFilename(
				Pkg->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
			UPackage::SavePackage(Pkg, Label, *DiskPath, SaveArgs);
		}
		else
		{
			UE_LOG(LogSN2ModTools, Warning, TEXT("Failed to create PAL_%s"), *ModName);
			bAllOk = false;
		}
	}

	if (bAllOk)
	{
		ShowNotification(
			FString::Printf(TEXT("Mod '%s' created (ChunkID %d)."), *ModName, ChunkId), true);
	}
	return bAllOk;
}

static FString GetRunUATPath()
{
	const USN2ModToolsSettings* Settings = GetDefault<USN2ModToolsSettings>();
	if (!Settings->RunUATOverride.FilePath.IsEmpty()
		&& FPaths::FileExists(Settings->RunUATOverride.FilePath))
	{
		return Settings->RunUATOverride.FilePath;
	}

	const FString EngineDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	const FString Candidate = EngineDir / TEXT("Build/BatchFiles/RunUAT.bat");
	if (FPaths::FileExists(Candidate)) return Candidate;

	return FString();
}

static int32 GetChunkIdForMod(const FString& ModName)
{
	IAssetRegistry& AR =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	const FString PALPath = FString::Printf(
		TEXT("/Game/Mods/%s/PAL_%s.PAL_%s"), *ModName, *ModName, *ModName);
	FAssetData Data = AR.GetAssetByObjectPath(FSoftObjectPath(PALPath));
	if (UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Data.GetAsset()))
	{
		return Label->Rules.ChunkId;
	}
	return -1;
}

static bool IsUE4SSInstalled(const FString& GameDir)
{
	const FString Win64 = GameDir / TEXT("Subnautica2/Binaries/Win64");
	const bool bHasProxy = FPaths::FileExists(Win64 / TEXT("dwmapi.dll"));
	const bool bHasLoader =
		IFileManager::Get().DirectoryExists(*(Win64 / TEXT("ue4ss/Mods/BPModLoaderMod")));
	return bHasProxy && bHasLoader;
}

static void MergeModsTxt(const FString& ModsTxtPath, const FString& PrevContents)
{
	if (PrevContents.IsEmpty()) return;

	FString Current;
	FFileHelper::LoadFileToString(Current, *ModsTxtPath);

	TSet<FString> Known;
	TArray<FString> CurLines;
	Current.ParseIntoArrayLines(CurLines);
	for (const FString& Line : CurLines)
	{
		const FString Trimmed = Line.TrimStartAndEnd();
		if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT(";"))) continue;
		FString Name, Flag;
		if (Trimmed.Split(TEXT(":"), &Name, &Flag))
		{
			Known.Add(Name.TrimStartAndEnd());
		}
	}

	FString Appended;
	TArray<FString> PrevLines;
	PrevContents.ParseIntoArrayLines(PrevLines);
	for (const FString& Line : PrevLines)
	{
		const FString Trimmed = Line.TrimStartAndEnd();
		if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT(";"))) continue;
		FString Name, Flag;
		if (!Trimmed.Split(TEXT(":"), &Name, &Flag)) continue;
		const FString TrimmedName = Name.TrimStartAndEnd();
		if (Known.Contains(TrimmedName)) continue;
		Appended += Trimmed + TEXT("\n");
		Known.Add(TrimmedName);
	}

	if (Appended.IsEmpty()) return;

	if (!Current.EndsWith(TEXT("\n"))) Current += TEXT("\n");
	Current += TEXT("\n; Preserved from previous install\n");
	Current += Appended;
	FFileHelper::SaveStringToFile(Current, *ModsTxtPath);
}

static bool DownloadAndInstallUE4SS(const FString& GameDir)
{
	const FString Win64 = GameDir / TEXT("Subnautica2/Binaries/Win64");
	if (!IFileManager::Get().DirectoryExists(*Win64))
	{
		UE_LOG(LogSN2ModTools, Error, TEXT("Game Win64 dir not found: %s"), *Win64);
		return false;
	}

	const FString ModsTxtPath = Win64 / TEXT("ue4ss/Mods/mods.txt");
	FString PrevModsTxt;
	FFileHelper::LoadFileToString(PrevModsTxt, *ModsTxtPath);

	const FString TempDir = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectIntermediateDir() / TEXT("SN2ModTools"));
	IFileManager::Get().MakeDirectory(*TempDir, true);
	const FString ZipPath = TempDir / TEXT("zDEV-UE4SS_SN2.zip");
	IFileManager::Get().Delete(*ZipPath, false, true);

	// /releases/latest/download/ redirects to the current asset, no API call.
	const FString Url = TEXT("https://github.com/Subnautica2Modding/")
		TEXT("Subnautica2-UE4SS/releases/latest/download/zDEV-UE4SS_SN2.zip");

	const FString PsCmd = FString::Printf(
		TEXT("$ProgressPreference='SilentlyContinue';")
		TEXT("Invoke-WebRequest -UseBasicParsing -Uri '%s' -OutFile '%s';")
		TEXT("Expand-Archive -Force -Path '%s' -DestinationPath '%s'"),
		*Url, *ZipPath, *ZipPath, *Win64);

	const FString PsArgs = FString::Printf(
		TEXT("-NoProfile -ExecutionPolicy Bypass -Command \"%s\""), *PsCmd);

	int32 ExitCode = -1;
	FString StdOut, StdErr;
	const bool bRan = FPlatformProcess::ExecProcess(
		TEXT("powershell.exe"), *PsArgs, &ExitCode, &StdOut, &StdErr);

	if (!bRan || ExitCode != 0)
	{
		UE_LOG(LogSN2ModTools, Error,
			TEXT("UE4SS install failed (exit %d): %s"), ExitCode, *StdErr);
		return false;
	}

	IFileManager::Get().Delete(*ZipPath, false, true);
	MergeModsTxt(ModsTxtPath, PrevModsTxt);

	UE_LOG(LogSN2ModTools, Display, TEXT("UE4SS installed to %s"), *Win64);
	return true;
}

static bool EnsureUE4SSReady(const FString& GameDir)
{
	if (IsUE4SSInstalled(GameDir)) return true;

	const FText Title = FText::FromString(TEXT("UE4SS not detected"));
	const FText Body = FText::FromString(
		TEXT("UE4SS and BPModLoaderMod are required for SN2 mods to load,")
		TEXT(" but they were not found in your game install.\n\n")
		TEXT("Install the official Subnautica2-UE4SS release now?\n\n")
		TEXT("(Downloads zDEV-UE4SS_SN2.zip from")
		TEXT(" github.com/Subnautica2Modding/Subnautica2-UE4SS)"));

	const EAppReturnType::Type Choice =
		FMessageDialog::Open(EAppMsgType::YesNo, Body, Title);
	if (Choice != EAppReturnType::Yes) return false;

	FNotificationInfo Info(FText::FromString(TEXT("Installing UE4SS...")));
	Info.bFireAndForget = false;
	Info.bUseThrobber = true;
	TSharedPtr<SNotificationItem> Notif =
		FSlateNotificationManager::Get().AddNotification(Info);
	if (Notif) Notif->SetCompletionState(SNotificationItem::CS_Pending);

	const bool bOk = DownloadAndInstallUE4SS(GameDir);

	if (Notif)
	{
		Notif->SetText(FText::FromString(
			bOk ? TEXT("UE4SS installed.") : TEXT("UE4SS install failed. See Output Log.")));
		Notif->SetCompletionState(
			bOk ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		Notif->ExpireAndFadeout();
	}
	return bOk;
}

void ModActions::CookAndInstallMod(const FString& ModName)
{
	const FString RunUAT = GetRunUATPath();
	if (RunUAT.IsEmpty())
	{
		ShowNotification(TEXT("RunUAT.bat not found. Check SN2 Mod Tools settings."), false);
		return;
	}

	const USN2ModToolsSettings* Settings = GetDefault<USN2ModToolsSettings>();
	if (Settings->GameDir.Path.IsEmpty())
	{
		ShowNotification(
			TEXT("Game directory not set. Open Project Settings > SN2 Mod Tools."), false);
		return;
	}

	if (!EnsureUE4SSReady(Settings->GameDir.Path))
	{
		ShowNotification(
			TEXT("Cook cancelled. UE4SS is required for SN2 mods to load."), false);
		return;
	}

	const int32 ChunkId = GetChunkIdForMod(ModName);
	if (ChunkId < 0)
	{
		ShowNotification(
			FString::Printf(TEXT("Cannot find PAL_%s or its ChunkID."), *ModName), false);
		return;
	}

	const FString ProjectPath =
		FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	const FString OutputDir =
		FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SN2Cook"));

	// -build compiles the Shipping target; staging needs its .target receipt.
	const FString Args = FString::Printf(
		TEXT("BuildCookRun"
			" -project=\"%s\""
			" -platform=Win64"
			" -clientconfig=Shipping"
			" -build -cook -stage -pak -iostore"
			" -archive -archivedirectory=\"%s\""
			" -nocompileeditor -installed"
			" -nop4 -utf8output -unattended"),
		*ProjectPath, *OutputDir);

	FNotificationInfo Info(
		FText::FromString(FString::Printf(TEXT("Cooking '%s'"), *ModName)));
	Info.bFireAndForget = false;
	Info.bUseThrobber = true;
	Info.bUseSuccessFailIcons = true;
	Info.FadeOutDuration = 3.f;
	Info.ExpireDuration = 6.f;

	TSharedPtr<SNotificationItem> Notification =
		FSlateNotificationManager::Get().AddNotification(Info);
	Notification->SetCompletionState(SNotificationItem::CS_Pending);

	TWeakPtr<SNotificationItem> WeakNotif = Notification;

	auto ParseUATLine = [](const FString& Line, FString& OutStatus) -> bool
	{
		if (Line.StartsWith(TEXT("[")))
		{
			int32 Slash, Close;
			if (Line.FindChar('/', Slash) && Line.FindChar(']', Close) && Close > Slash)
			{
				const FString Rest = Line.Mid(Close + 1).TrimStart();
				if (!Rest.IsEmpty())
				{
					OutStatus = Line.Left(Close + 1) + TEXT(" ") + Rest.Left(60);
					return true;
				}
			}
		}
		if (Line.Contains(TEXT("LogCook:")) && Line.Contains(TEXT("Cooking")))
		{
			OutStatus = TEXT("Cooking assets...");
			return true;
		}
		if (Line.Contains(TEXT("LogCook:")) && Line.Contains(TEXT("Finish")))
		{
			OutStatus = TEXT("Finalising cook...");
			return true;
		}
		if (Line.Contains(TEXT("Stage:")) || Line.Contains(TEXT("Staging")))
		{
			OutStatus = TEXT("Staging files...");
			return true;
		}
		if (Line.Contains(TEXT("Pak:")) || Line.Contains(TEXT("UnrealPak")))
		{
			OutStatus = TEXT("Building pak...");
			return true;
		}
		if (Line.Contains(TEXT("Archive:")))
		{
			OutStatus = TEXT("Archiving...");
			return true;
		}
		return false;
	};

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
		[RunUAT, Args, OutputDir, ModName, ChunkId, GameDir = Settings->GameDir.Path,
		 WeakNotif, ParseUATLine]()
	{
		auto UpdateNotif = [&WeakNotif](const FString& Text)
		{
			AsyncTask(ENamedThreads::GameThread, [WeakNotif, Text]()
			{
				if (TSharedPtr<SNotificationItem> Pin = WeakNotif.Pin())
				{
					Pin->SetText(FText::FromString(Text));
				}
			});
		};

		auto FinishNotif = [&WeakNotif](const FString& Text, bool bSuccess)
		{
			AsyncTask(ENamedThreads::GameThread, [WeakNotif, Text, bSuccess]()
			{
				if (TSharedPtr<SNotificationItem> Pin = WeakNotif.Pin())
				{
					Pin->SetText(FText::FromString(Text));
					Pin->SetCompletionState(
						bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
					Pin->ExpireAndFadeout();
				}
			});
		};

		void* PipeRead  = nullptr;
		void* PipeWrite = nullptr;
		FPlatformProcess::CreatePipe(PipeRead, PipeWrite);

		FProcHandle Proc = FPlatformProcess::CreateProc(
			*RunUAT, *Args,
			false, true, true,
			nullptr, 0, nullptr, PipeWrite, PipeRead);

		if (!Proc.IsValid())
		{
			FPlatformProcess::ClosePipe(PipeRead, PipeWrite);
			FinishNotif(TEXT("Failed to launch RunUAT."), false);
			return;
		}

		FString LastStatus;
		while (FPlatformProcess::IsProcRunning(Proc))
		{
			const FString Chunk = FPlatformProcess::ReadPipe(PipeRead);
			if (!Chunk.IsEmpty())
			{
				UE_LOG(LogSN2ModTools, Log, TEXT("[UAT] %s"), *Chunk);

				TArray<FString> Lines;
				Chunk.ParseIntoArrayLines(Lines);
				for (const FString& Line : Lines)
				{
					FString Status;
					if (ParseUATLine(Line.TrimStartAndEnd(), Status) && Status != LastStatus)
					{
						LastStatus = Status;
						UpdateNotif(FString::Printf(
							TEXT("Cooking '%s'\n%s"), *ModName, *Status));
					}
				}
			}
			FPlatformProcess::Sleep(0.2f);
		}

		int32 ExitCode = 0;
		FPlatformProcess::GetProcReturnCode(Proc, &ExitCode);
		FPlatformProcess::ClosePipe(PipeRead, PipeWrite);
		FPlatformProcess::CloseProc(Proc);

		if (ExitCode != 0)
		{
			FinishNotif(
				FString::Printf(TEXT("Cook failed (exit %d). See Output Log."), ExitCode), false);
			return;
		}

		UpdateNotif(FString::Printf(TEXT("Cooking '%s'\nInstalling..."), *ModName));

		const FString PaksDir = OutputDir / TEXT("Windows/Subnautica2/Content/Paks");
		const FString Pattern = FString::Printf(TEXT("pakchunk%d-*"), ChunkId);

		TArray<FString> PakFiles;
		IFileManager::Get().FindFiles(PakFiles, *(PaksDir / Pattern), true, false);

		if (PakFiles.IsEmpty())
		{
			FinishNotif(FString::Printf(
				TEXT("Cook succeeded but no pakchunk found for '%s'."), *ModName), false);
			return;
		}

		const FString ModInstallDir =
			GameDir / TEXT("Subnautica2/Content/Paks/LogicMods") / ModName;
		IFileManager::Get().MakeDirectory(*ModInstallDir, true);

		int32 Installed = 0;
		for (const FString& File : PakFiles)
		{
			const FString Src  = PaksDir / File;
			const FString Dest =
				ModInstallDir / (ModName + TEXT(".") + FPaths::GetExtension(File));
			if (IFileManager::Get().Copy(*Dest, *Src) == COPY_OK)
			{
				++Installed;
				UE_LOG(LogSN2ModTools, Display, TEXT("Installed: %s"), *Dest);
			}
			else
			{
				UE_LOG(LogSN2ModTools, Warning, TEXT("Failed to copy: %s -> %s"), *Src, *Dest);
			}
		}

		FinishNotif(
			FString::Printf(TEXT("'%s' installed (%d file(s))"), *ModName, Installed), true);
	});
}

void ModActions::UninstallMod(const FString& ModName)
{
	const USN2ModToolsSettings* Settings = GetDefault<USN2ModToolsSettings>();
	if (Settings->GameDir.Path.IsEmpty())
	{
		ShowNotification(
			TEXT("Game directory not set. Open Project Settings > SN2 Mod Tools."), false);
		return;
	}

	const FString ModInstallDir =
		Settings->GameDir.Path / TEXT("Subnautica2/Content/Paks/LogicMods") / ModName;

	if (!IFileManager::Get().DirectoryExists(*ModInstallDir))
	{
		ShowNotification(
			FString::Printf(TEXT("No installed files found for '%s'."), *ModName), false);
		return;
	}

	const bool bRemoved =
		IFileManager::Get().DeleteDirectory(*ModInstallDir, false, true);

	ShowNotification(
		bRemoved
			? FString::Printf(TEXT("'%s' uninstalled."), *ModName)
			: FString::Printf(TEXT("Failed to remove '%s'. Is the game running?"), *ModName),
		bRemoved);
}

FString ModActions::ModNameFromFolderPath(const FString& FolderPath)
{
	const FString Prefix = TEXT("/Game/Mods/");
	if (!FolderPath.StartsWith(Prefix)) return FString();
	const FString Remainder = FolderPath.Mid(Prefix.Len());
	if (Remainder.Contains(TEXT("/"))) return FString();
	return Remainder;
}
