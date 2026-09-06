// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AssetRegistry/AssetData.h"

#include "DragAndDrop/AssetDragDropOp.h"

enum class EDragAssetType
{
	Mesh,
	Material,
	Decal
};

class FFabDragDropOp : public FAssetDragDropOp
{
public:
	DECLARE_DELEGATE(FOnDrop);
	DRAG_DROP_OPERATOR_TYPE(FFabDragDropOp, FAssetDragDropOp)

	static TSharedPtr<FFabDragDropOp> New(FAssetData Asset, EDragAssetType InDragAssetType);

	FFabDragDropOp(const EDragAssetType InDragAssetType);
	virtual ~FFabDragDropOp() override;

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
	FOnDrop& OnDrop() { return this->OnDropDelegate; }

	void SetCanDropHere(bool bCanDropHere)
	{
		MouseCursor = bCanDropHere ? EMouseCursor::GrabHandClosed : EMouseCursor::SlashedCircle;
	}

	virtual void Construct() override;

	void Cancel();
	void DestroyWindow();
	void DestroySpawnedActor();

	virtual void OnDragged(const FDragDropEvent& DragDropEvent) override;
	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override;

	TWeakObjectPtr<AActor> SpawnedActor;

protected:
	FOnDrop OnDropDelegate;

private:
	EDragAssetType DragAssetType;
	FDelegateHandle EditorApplyHandle;
};
