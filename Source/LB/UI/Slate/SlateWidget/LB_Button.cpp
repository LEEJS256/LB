// Fill out your copyright notice in the Description page of Project Settings.

#include "LB_Button.h"

#include "ui/Slate/SLB_Button.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

ULB_Button::ULB_Button()
{
	Font = FCoreStyle::GetDefaultFontStyle("Bold", 16);
}

TSharedRef<SLB_SquareBox> ULB_Button::CreateSquareBox()
{
	// 부모 기본(SLB_SquareBox)은 클릭을 못 받아서 SLB_Button으로 교체
	return SNew(SLB_Button)
		.OnClicked(FOnClicked::CreateUObject(this, &ULB_Button::HandleSlateClicked));
}

TSharedRef<SWidget> ULB_Button::RebuildWidget()
{
	TSharedRef<SWidget> Result = Super::RebuildWidget();

	MyLabel.Reset();
	const bool bHasChild = GetContentSlot() && GetContentSlot()->Content;
	if (!bHasChild && MySquareBox.IsValid())
	{
		// 자식이 없으면 라벨을 기본 내용으로 넣는다. 마우스는 버튼이 받도록 HitTestInvisible
		MySquareBox->SetContent(
			SNew(SBox)
			.Visibility(EVisibility::HitTestInvisible)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SAssignNew(MyLabel, STextBlock)
				.Text(LabelText)
				.Font(Font)
				.ColorAndOpacity_Lambda([this]()
				{
					const bool bHover = MySquareBox.IsValid() && MySquareBox->IsHovered();
					return FSlateColor(FLinearColor::FromSRGBColor(bHover ? HoverTextColor : TextColor));
				})
			]);
	}
	return Result;
}

void ULB_Button::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyLabel();
}

void ULB_Button::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyLabel.Reset();
}

void ULB_Button::ApplyLabel()
{
	if (MyLabel.IsValid())
	{
		MyLabel->SetText(LabelText);
		MyLabel->SetFont(Font);
	}
}

FReply ULB_Button::HandleSlateClicked()
{
	OnClicked.Broadcast();
	return FReply::Handled();
}

void ULB_Button::SetLabelText(const FText& InText)
{
	LabelText = InText;
	ApplyLabel();
}

void ULB_Button::SetFont(const FSlateFontInfo& InFont)
{
	Font = InFont;
	ApplyLabel();
}

void ULB_Button::SetTextColor(FColor InColor)
{
	TextColor = InColor;   // 색은 람다가 매번 읽으니 따로 적용할 것이 없다
}

void ULB_Button::SetBorderColor(FLinearColor InColor)
{
	FLB_SquareBoxStyle S = Style;
	S.BorderColor = InColor;
	SetStyle(S);
}

void ULB_Button::SetBorderThickness(float InThickness)
{
	FLB_SquareBoxStyle S = Style;
	S.BorderThickness = InThickness;
	SetStyle(S);
}