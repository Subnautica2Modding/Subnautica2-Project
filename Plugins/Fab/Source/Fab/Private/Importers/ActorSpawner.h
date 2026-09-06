// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ActorFactories/ActorFactory.h"

#include "AssetRegistry/AssetData.h"

#include "ActorSpawner.generated.h"

UCLASS()
class UFabPlaceholderSpawner : public UActorFactory
{
	GENERATED_UCLASS_BODY()

public:
	DECLARE_DELEGATE_OneParam(FOnActorSpawn, AActor*);
	FOnActorSpawn& OnActorSpawn() { return this->OnActorSpawnDelegate; }

protected:
	FOnActorSpawn OnActorSpawnDelegate;
};

UCLASS()
class UFabStaticMeshPlaceholderSpawner : public UFabPlaceholderSpawner
{
	GENERATED_UCLASS_BODY()

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* Instance) override;
};

UCLASS()
class UFabSkeletalMeshPlaceholderSpawner : public UFabPlaceholderSpawner
{
	GENERATED_UCLASS_BODY()

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* Instance) override;
};

UCLASS()
class UFabDecalPlaceholderSpawner : public UFabPlaceholderSpawner
{
	GENERATED_UCLASS_BODY()

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* Instance) override;
};
