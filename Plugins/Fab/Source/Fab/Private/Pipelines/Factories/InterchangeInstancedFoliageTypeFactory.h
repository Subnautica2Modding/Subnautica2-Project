// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "InterchangeFactoryBase.h"

#include "InterchangeInstancedFoliageTypeFactory.generated.h"

UCLASS()
class UInterchangeInstancedFoliageTypeFactory : public UInterchangeFactoryBase
{
	GENERATED_BODY()

public:
	virtual UClass* GetFactoryClass() const override;
	virtual EInterchangeFactoryAssetType GetFactoryAssetType() override { return EInterchangeFactoryAssetType::Meshes; }
	virtual FImportAssetResult BeginImportAsset_GameThread(const FImportAssetObjectParams& Arguments) override;
	virtual void SetupObject_GameThread(const FSetupObjectParams& Arguments) override;
};
