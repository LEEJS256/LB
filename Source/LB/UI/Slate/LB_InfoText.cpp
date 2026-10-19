#include "LB_InfoText.h"
#include "SLB_InfoText.h"

TSharedRef<SWidget> ULB_InfoText::RebuildWidget()
{
	MyInfoText = SNew(SLB_InfoText)
		.IconText(IconText)
		.LabelText(LabelText)
		.ValueText(ValueText)
		.IconColor(FSlateColor(IconColor))
		.LabelFontSize(LabelFontSize)
		.ValueFontSize(ValueFontSize)
		.LabelColor(FSlateColor(LabelColor))
		.ValueColor(FSlateColor(ValueColor))
		.BoxColor(FSlateColor(BoxColor))
		.BoxPadding(BoxPadding)
		.IconFontSize(IconFontSize)
		.IconBoxSize(IconBoxSize);

	return MyInfoText.ToSharedRef();
}

void ULB_InfoText::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyInfoText.IsValid())
	{
		MyInfoText->SetLabelColor(FSlateColor(LabelColor));
		MyInfoText->SetValueColor(FSlateColor(ValueColor));
		MyInfoText->SetValue(ValueText);
		MyInfoText->SetLabelText(LabelText);
		MyInfoText->SetLabelFontSize(LabelFontSize);
		MyInfoText->SetValueFontSize(ValueFontSize);
		MyInfoText->SetIconBoxSize(IconBoxSize);
		MyInfoText->SetBoxColor(FSlateColor(BoxColor));
		MyInfoText->SetBoxPadding(BoxPadding);
		MyInfoText->SetIconFontSize(IconFontSize);
	}
}

void ULB_InfoText::SetBoxColor(FLinearColor InColor)
{
	BoxColor = InColor;
	if (MyInfoText.IsValid()) MyInfoText->SetBoxColor(FSlateColor(InColor));
}

void ULB_InfoText::SetIconFontSize(int32 InSize)
{
	IconFontSize = InSize;
	if (MyInfoText.IsValid()) MyInfoText->SetIconFontSize(InSize);
}

void ULB_InfoText::SetLabelColor(FLinearColor InColor)
{
	LabelColor = InColor;
	if (MyInfoText.IsValid()) MyInfoText->SetLabelColor(FSlateColor(InColor));
}

void ULB_InfoText::SetValueColor(FLinearColor InColor)
{
	ValueColor = InColor;
	if (MyInfoText.IsValid()) MyInfoText->SetValueColor(FSlateColor(InColor));
}

void ULB_InfoText::SetLabelFontSize(int32 InSize)
{
	LabelFontSize = InSize;
	if (MyInfoText.IsValid()) MyInfoText->SetLabelFontSize(InSize);
}

void ULB_InfoText::SetValueFontSize(int32 InSize)
{
	ValueFontSize = InSize;
	if (MyInfoText.IsValid()) MyInfoText->SetValueFontSize(InSize);
}

void ULB_InfoText::SetValue(const FText& InValue)
{
	ValueText = InValue;
	if (MyInfoText.IsValid())
	{
		MyInfoText->SetValue(ValueText);
	}
}

void ULB_InfoText::SetValueInt(int32 InValue)
{
	SetValue(FText::AsNumber(InValue));
}

void ULB_InfoText::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyInfoText.Reset();
}

#if WITH_EDITOR
const FText ULB_InfoText::GetPaletteCategory()
{
	return FText::FromString(TEXT("LB UI Custom"));
}
#endif
