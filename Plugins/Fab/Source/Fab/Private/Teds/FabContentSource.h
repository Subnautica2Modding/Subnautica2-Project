// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ContentSources/IContentSource.h"

class FFabMyLibraryContentSource : public UE::Editor::ContentBrowser::IContentSource
{
public:
	virtual ~FFabMyLibraryContentSource() override = default;

	virtual FName GetName() override;
	virtual FText GetDisplayName() override;
	virtual FSlateIcon GetIcon() override;
	virtual void GetAssetViewInitParams(UE::Editor::ContentBrowser::FTableViewerInitParams& OutInitParams) override;
};