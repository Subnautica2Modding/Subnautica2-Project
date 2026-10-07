// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FFabDownloadRequest;
struct FFabDownloadStats;

class IFabWorkflow
{
public:
	DECLARE_DELEGATE(FOnFabWorkflowComplete);
	DECLARE_DELEGATE(FOnFabWorkflowCancel);

public:
	IFabWorkflow(const FString& InAssetId, const FString InAssetName, const FString& InDownloadUrl)
		: AssetId(InAssetId)
		, AssetName(InAssetName)
		, DownloadUrl(InDownloadUrl)
	{}

	virtual ~IFabWorkflow() = default;

	virtual void Execute() = 0;

	FOnFabWorkflowComplete& OnFabWorkflowComplete() { return OnFabWorkflowCompleteDelegate; }
	FOnFabWorkflowCancel& OnFabWorkflowCancel() { return OnFabWorkflowCancelDelegate; }

	virtual const TArray<UObject*>& GetImportedObjects() const { return ImportedObjects; }

	template <class T>
	T* GetImportedObjectOfType() const
	{
		if (UObject* const* FoundObject = ImportedObjects.FindByPredicate([](const auto& O) { return O->IsA(T::StaticClass()); }))
		{
			return Cast<T>(*FoundObject);
		}
		return nullptr;
	}

protected:
	virtual void ImportContent(const TArray<FString>& SourceFiles) {};
	virtual void DownloadContent() = 0;

	virtual void OnContentDownloadProgress(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) = 0;
	virtual void OnContentDownloadComplete(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) = 0;

	virtual void CompleteWorkflow() { OnFabWorkflowComplete().ExecuteIfBound(); }
	virtual void CancelWorkflow() { OnFabWorkflowCancel().ExecuteIfBound(); }

public:
	FString AssetId;

protected:
	FString AssetName;
	FString DownloadUrl;

	FString ImportLocation;
	TArray<UObject*> ImportedObjects;

private:
	FOnFabWorkflowComplete OnFabWorkflowCompleteDelegate;
	FOnFabWorkflowCancel OnFabWorkflowCancelDelegate;
};
