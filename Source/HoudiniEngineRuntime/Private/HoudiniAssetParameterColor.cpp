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

#include "HoudiniEngineRuntimePrivatePCH.h"
#include "HoudiniAssetParameterColor.h"
#include "HoudiniApi.h"


UHoudiniAssetParameterColor::UHoudiniAssetParameterColor(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	Color = FColor::White;
}


UHoudiniAssetParameterColor::~UHoudiniAssetParameterColor()
{

}


void
UHoudiniAssetParameterColor::Serialize(FArchive& Ar)
{
	// Call base implementation.
	Super::Serialize(Ar);

	if(Ar.IsLoading())
	{
		Color = FColor::White;
	}

	Ar << Color;
}


UHoudiniAssetParameterColor*
UHoudiniAssetParameterColor::Create(UHoudiniAssetComponent* InHoudiniAssetComponent,
	UHoudiniAssetParameter* InParentParameter, HAPI_NodeId InNodeId, const HAPI_ParmInfo& ParmInfo)
{
	UObject* Outer = InHoudiniAssetComponent;
	if(!Outer)
	{
		Outer = InParentParameter;
		if(!Outer)
		{
			// Must have either component or parent not null.
			check(false);
		}
	}

	UHoudiniAssetParameterColor* HoudiniAssetParameterColor = NewObject<UHoudiniAssetParameterColor>(Outer,
		UHoudiniAssetParameterColor::StaticClass(), NAME_None, RF_Public | RF_Transactional);

	HoudiniAssetParameterColor->CreateParameter(InHoudiniAssetComponent, InParentParameter, InNodeId, ParmInfo);
	return HoudiniAssetParameterColor;
}


bool
UHoudiniAssetParameterColor::CreateParameter(UHoudiniAssetComponent* InHoudiniAssetComponent,
	UHoudiniAssetParameter* InParentParameter, HAPI_NodeId InNodeId, const HAPI_ParmInfo& ParmInfo)
{
	if(!Super::CreateParameter(InHoudiniAssetComponent, InParentParameter, InNodeId, ParmInfo))
	{
		return false;
	}

	// We can only handle float type.
	if(HAPI_PARMTYPE_COLOR != ParmInfo.type)
	{
		return false;
	}

	// Assign internal Hapi values index.
	SetValuesIndex(ParmInfo.floatValuesIndex);

	// Get the actual value for this property.
	Color = FLinearColor::White;
	if(HAPI_RESULT_SUCCESS != FHoudiniApi::GetParmFloatValues(
		FHoudiniEngine::Get().GetSession(), InNodeId, (float*)&Color.R, ValuesIndex, TupleSize))
	{
		return false;
	}

	if(TupleSize == 3)
	{
		Color.A = 1.0f;
	}

	return true;
}


#if WITH_EDITOR

void
UHoudiniAssetParameterColor::CreateWidget(IDetailCategoryBuilder& DetailCategoryBuilder)
{
	ColorBlock.Reset();

	Super::CreateWidget(DetailCategoryBuilder);

	FDetailWidgetRow& Row = DetailCategoryBuilder.AddCustomRow(FText::GetEmpty());
	
	// Create the standard parameter name widget.
	CreateNameWidget(Row, true);

	ColorBlock = SNew(SColorBlock);
	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);
	VerticalBox->AddSlot().Padding(2, 2, 5, 2)
	[
		SAssignNew(ColorBlock, SColorBlock)
		.Color(TAttribute<FLinearColor>::Create(TAttribute<FLinearColor>::FGetter::CreateUObject(this,
			&UHoudiniAssetParameterColor::GetColor)))
		.OnMouseButtonDown(FPointerEventHandler::CreateUObject(this,
			&UHoudiniAssetParameterColor::OnColorBlockMouseButtonDown))
	];

	if(ColorBlock.IsValid())
	{
		ColorBlock->SetEnabled(!bIsDisabled);
	}

	Row.ValueWidget.Widget = VerticalBox;
	Row.ValueWidget.MinDesiredWidth(HAPI_UNREAL_DESIRED_ROW_VALUE_WIDGET_WIDTH);
}

#endif


bool
UHoudiniAssetParameterColor::UploadParameterValue()
{
	if(HAPI_RESULT_SUCCESS != FHoudiniApi::SetParmFloatValues(
		FHoudiniEngine::Get().GetSession(), NodeId, (const float*)&Color.R, ValuesIndex, TupleSize))
	{
		return false;
	}

	return Super::UploadParameterValue();
}


FLinearColor
UHoudiniAssetParameterColor::GetColor() const
{
	return Color;
}


#if WITH_EDITOR

FReply
UHoudiniAssetParameterColor::OnColorBlockMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if(MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	FColorPickerArgs PickerArgs;
	PickerArgs.ParentWidget = ColorBlock;
	PickerArgs.bUseAlpha = true;
	PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine,
		&UEngine::GetDisplayGamma));
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateUObject(this,
		&UHoudiniAssetParameterColor::OnPaintColorChanged);
	PickerArgs.InitialColorOverride = GetColor();
	PickerArgs.bOnlyRefreshOnOk = true;

	OpenColorPicker(PickerArgs);

	return FReply::Handled();
}

#endif


void
UHoudiniAssetParameterColor::OnPaintColorChanged(FLinearColor InNewColor)
{
	if(Color != InNewColor)
	{

#if WITH_EDITOR

		// Record undo information.
		FScopedTransaction Transaction(TEXT(HOUDINI_MODULE_RUNTIME),
			LOCTEXT("HoudiniAssetParameterColorChange", "Houdini Parameter Color: Changing a value"),
			HoudiniAssetComponent);
		Modify();

#endif

		MarkPreChanged();

		Color = InNewColor;

		// Mark this parameter as changed.
		MarkChanged();
	}
}
