// Copyright Epic Games, Inc. All Rights Reserved.

#include "FabSettingsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "FabSettings.h"
#include "DetailWidgetRow.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"

#include "PropertyEditing.h"

#include "Utilities/FabAssetsCache.h"

TSharedRef<IDetailCustomization> FFabSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FFabSettingsCustomization);
}

void FFabSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& GeneralCategory = DetailBuilder.EditCategory("General");
	IDetailCategoryBuilder& FormatsCategory = DetailBuilder.EditCategory("ProductFormats");

	// General section modifications

	DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UFabSettings, CacheDirectorySize));
	TArray<TSharedRef<IPropertyHandle>> GeneralProperties;
	GeneralCategory.GetDefaultProperties(GeneralProperties);
	for (const TSharedRef<IPropertyHandle>& PropertyHandle : GeneralProperties)
	{
		// Skip the already customized NonEditableString property
		if (PropertyHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UFabSettings, CacheDirectorySize))
		{
			continue;
		}

		// Add the property
		GeneralCategory.AddProperty(PropertyHandle);
	}

	const TSharedRef<IPropertyHandle> CacheSizeStringHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFabSettings, CacheDirectorySize));
	GeneralCategory.AddCustomRow(CacheSizeStringHandle->GetPropertyDisplayName())
	.NameContent()
	[
		CacheSizeStringHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SBox)
		.MinDesiredWidth(1400.f)
		.HAlign(HAlign_Fill)
		[
			SNew(SOverlay)
			.FlowDirectionPreference(EFlowDirectionPreference::LeftToRight)

			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(SEditableTextBox)
				.Text_Static(&FFabAssetsCache::GetCacheSizeString)
				.IsReadOnly(true)
			]
			
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(SButton).Text(FText::FromString("Clean Directory"))
				.OnClicked(FOnClicked::CreateRaw(this, &FFabSettingsCustomization::OnButtonClick))
			]
		]
	];

	// Product format section modifications

	// DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UFabSettings, ProductFormatsSectionSubText));
	// const TSharedRef<IPropertyHandle> ProductFormatSubTextHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFabSettings, ProductFormatsSectionSubText));
	// FormatsCategory.AddCustomRow(ProductFormatSubTextHandle->GetPropertyDisplayName())
	// .WholeRowContent()
	// [
	// 	SNew(SHorizontalBox)
	// 	+ SHorizontalBox::Slot()
	// 	.HAlign(HAlign_Fill)
	// 	.FillWidth(1.0f)
	// 	[
	// 		SNew(STextBlock)
	// 		.Text(FText::FromString("The preferred format will always be selected, if not available, the best available format for the product will be chosen."))
	// 	]
	// ]
	// .ValueContent();
}

FReply FFabSettingsCustomization::OnButtonClick()
{
	FFabAssetsCache::ClearCache();
	return FReply::Handled();
}
