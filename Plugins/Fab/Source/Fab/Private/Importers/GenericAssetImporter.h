// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UFbxImportUI;
class UInterchangePipelineStackOverride;

class FFabGenericImporter
{
private:
	static UObject* GetImportOptions(const FString& SourceFile, UObject* const OptionsOuter);
	static void CleanImportOptions(UObject* const Options);

public:
	static void ImportAsset(const TArray<FString>& Sources, const FString& Destination, const TFunction<void(const TArray<UObject*>&)>& Callback);
};
