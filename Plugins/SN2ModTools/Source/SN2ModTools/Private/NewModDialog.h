#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"

class SNewModDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNewModDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static FString ShowModal();

private:
	FReply OnOKClicked();
	FReply OnCancelClicked();
	bool IsOKEnabled() const;

	TSharedPtr<SEditableTextBox> NameBox;
	TWeakPtr<SWindow>            ParentWindow;
	FString                      Result;
	bool                         bConfirmed = false;
};
