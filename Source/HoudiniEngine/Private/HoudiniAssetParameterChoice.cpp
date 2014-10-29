/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *      Mykola Konyk
 *      Side Effects Software Inc
 *      123 Front Street West, Suite 1401
 *      Toronto, Ontario
 *      Canada   M5J 2M2
 *      416-504-9876
 *
 */

#include "HoudiniEnginePrivatePCH.h"


UHoudiniAssetParameterChoice::UHoudiniAssetParameterChoice(const FPostConstructInitializeProperties& PCIP) :
	Super(PCIP),
	CurrentValue(0),
	bStringChoiceList(false)
{
	StringValue = TEXT("");
}


UHoudiniAssetParameterChoice::~UHoudiniAssetParameterChoice()
{

}


UHoudiniAssetParameterChoice*
UHoudiniAssetParameterChoice::Create(UHoudiniAssetComponent* InHoudiniAssetComponent, HAPI_NodeId InNodeId, const HAPI_ParmInfo& ParmInfo)
{
	UHoudiniAssetParameterChoice* HoudiniAssetParameterChoice = new UHoudiniAssetParameterChoice(FPostConstructInitializeProperties());
	HoudiniAssetParameterChoice->CreateParameter(InHoudiniAssetComponent, InNodeId, ParmInfo);
	return HoudiniAssetParameterChoice;
}


bool
UHoudiniAssetParameterChoice::CreateParameter(UHoudiniAssetComponent* InHoudiniAssetComponent, HAPI_NodeId InNodeId, const HAPI_ParmInfo& ParmInfo)
{
	if(!Super::CreateParameter(InHoudiniAssetComponent, InNodeId, ParmInfo))
	{
		return false;
	}

	// Choice lists can be only integer or string.
	if(HAPI_PARMTYPE_INT != ParmInfo.type && HAPI_PARMTYPE_STRING != ParmInfo.type)
	{
		return false;
	}

	// Get the actual value for this property.
	CurrentValue = 0;

	if(HAPI_PARMTYPE_INT == ParmInfo.type)
	{
		// This is an integer choice list.
		bStringChoiceList = false;

		// Assign internal Hapi values index.
		SetValuesIndex(ParmInfo.intValuesIndex);

		if(HAPI_RESULT_SUCCESS != HAPI_GetParmIntValues(NodeId, &CurrentValue, ValuesIndex, TupleSize))
		{
			return false;
		}
	}
	else if(HAPI_PARMTYPE_STRING == ParmInfo.type)
	{
		// This is a string choice list.
		bStringChoiceList = true;

		// Assign internal Hapi values index.
		SetValuesIndex(ParmInfo.stringValuesIndex);

		HAPI_StringHandle StringHandle;
		if(HAPI_RESULT_SUCCESS != HAPI_GetParmStringValues(NodeId, false, &StringHandle, ValuesIndex, TupleSize))
		{
			return false;
		}

		// Get the actual string value.
		if(!FHoudiniEngineUtils::GetHoudiniString(StringHandle, StringValue))
		{
			return false;
		}
	}

	// Get choice descriptors.
	TArray<HAPI_ParmChoiceInfo> ParmChoices;
	ParmChoices.SetNumZeroed(ParmInfo.choiceCount);
	if(HAPI_RESULT_SUCCESS != HAPI_GetParmChoiceLists(NodeId, &ParmChoices[0], ParmInfo.choiceIndex, ParmInfo.choiceCount))
	{
		return false;
	}

	// Get string values for all available choices.
	StringChoiceValues.Empty();
	StringChoiceLabels.Empty();

	bool bMatchedSelectionLabel = false;
	for(int ChoiceIdx = 0; ChoiceIdx < ParmChoices.Num(); ++ChoiceIdx)
	{
		FString* ChoiceValue = new FString();
		FString* ChoiceLabel = new FString();

		if(!FHoudiniEngineUtils::GetHoudiniString(ParmChoices[ChoiceIdx].valueSH, *ChoiceValue))
		{
			return false;
		}

		StringChoiceValues.Add(TSharedPtr<FString>(ChoiceValue));

		if(!FHoudiniEngineUtils::GetHoudiniString(ParmChoices[ChoiceIdx].labelSH, *ChoiceLabel))
		{
			*ChoiceLabel = *ChoiceValue;
		}

		StringChoiceLabels.Add(TSharedPtr<FString>(ChoiceLabel));

		// If this is a string choice list, we need to match name with corresponding selection label.
		if(bStringChoiceList && !bMatchedSelectionLabel && ChoiceValue->Equals(StringValue))
		{
			StringValue = *ChoiceLabel;
			bMatchedSelectionLabel = true;
			CurrentValue = ChoiceIdx;
		}
		else if(!bStringChoiceList && ChoiceIdx == CurrentValue)
		{
			StringValue = *ChoiceLabel;
		}
	}

	return true;
}


void
UHoudiniAssetParameterChoice::CreateWidget(IDetailCategoryBuilder& DetailCategoryBuilder)
{
	Super::CreateWidget(DetailCategoryBuilder);

	FDetailWidgetRow& Row = DetailCategoryBuilder.AddCustomRow(TEXT(""));

	Row.NameWidget.Widget = SNew(STextBlock)
							.Text(Label)
							.ToolTipText(Label)
							.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));

	Row.ValueWidget.Widget = SNew(SComboBox<TSharedPtr<FString> >)
							 .OptionsSource(&StringChoiceLabels)
							 .InitiallySelectedItem(StringChoiceLabels[CurrentValue])
							 .OnGenerateWidget(SComboBox<TSharedPtr<FString> >::FOnGenerateWidget::CreateUObject(this, &UHoudiniAssetParameterChoice::CreateChoiceEntryWidget))
							 .OnSelectionChanged(SComboBox<TSharedPtr<FString> >::FOnSelectionChanged::CreateUObject(this, &UHoudiniAssetParameterChoice::OnChoiceChance))
							 [
								SNew(STextBlock)
								.Text(TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateUObject(this, &UHoudiniAssetParameterChoice::HandleChoiceContentText)))
								.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							 ];
}


bool
UHoudiniAssetParameterChoice::UploadParameterValue()
{
	if(bStringChoiceList)
	{
		// Get corresponding value.
		FString* ChoiceValue = StringChoiceValues[CurrentValue].Get();
		std::string String = TCHAR_TO_UTF8(*(*ChoiceValue));
		HAPI_SetParmStringValue(NodeId, String.c_str(), ParmId, 0);
	}
	else
	{
		// This is an int choice list.
		HAPI_SetParmIntValues(NodeId, &CurrentValue, ValuesIndex, TupleSize);
	}

	return Super::UploadParameterValue();
}


void
UHoudiniAssetParameterChoice::Serialize(FArchive& Ar)
{
	// Call base implementation.
	Super::Serialize(Ar);
}


void
UHoudiniAssetParameterChoice::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	// Call base implementation.
	Super::AddReferencedObjects(InThis, Collector);
}


TSharedRef<SWidget>
UHoudiniAssetParameterChoice::CreateChoiceEntryWidget(TSharedPtr<FString> ChoiceEntry)
{
	return SNew(STextBlock)
		   .Text(*ChoiceEntry)
		   .ToolTipText(*ChoiceEntry)
		   .Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}


void
UHoudiniAssetParameterChoice::OnChoiceChance(TSharedPtr<FString> NewChoice, ESelectInfo::Type SelectType)
{
	if(!NewChoice.IsValid())
	{
		return;
	}

	StringValue = *(NewChoice.Get());

	// We need to match selection based on label.
	for(int32 LabelIdx = 0; LabelIdx < StringChoiceLabels.Num(); ++LabelIdx)
	{
		FString* ChoiceLabel = StringChoiceLabels[LabelIdx].Get();

		if(ChoiceLabel->Equals(StringValue))
		{
			CurrentValue = LabelIdx;
			break;
		}
	}

	// Mark this property as changed.
	MarkChanged();
}


FString
UHoudiniAssetParameterChoice::HandleChoiceContentText() const
{
	return StringValue;
}
