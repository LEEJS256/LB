#include "SLB_SquareBox.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SToolTip.h"
#include "Components/Widget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Materials/MaterialInterface.h"

void SLB_SquareBox::Construct(const FArguments& InArgs)
{
	NormalColor = InArgs._NormalColor;
	HoverColor = InArgs._HoverColor;
	PressedColor = InArgs._PressedColor;
	SelectedColor = InArgs._SelectedColor;
	HighlightColor = InArgs._HighlightColor;
	SelectedAttr = InArgs._Selected;
	BorderColor = InArgs._BorderColor;

	ChildSlot
	[
		SAssignNew(SizeBox, SBox)
		.WidthOverride(InArgs._BoxSize.X)
		.HeightOverride(InArgs._BoxSize.Y)
		[
			// 바깥: 테두리
			SAssignNew(OuterBorder, SBorder)
			.BorderImage(this, &SLB_SquareBox::GetBorderBrush)
			.BorderBackgroundColor(this, &SLB_SquareBox::GetBorderColor)
			.Padding(InArgs._BorderThickness)
			[
				// 안쪽: 상태색 배경 + 콘텐츠
				SAssignNew(FillBorder, SBorder)
				.BorderImage(this, &SLB_SquareBox::GetFillBrush)
				.BorderBackgroundColor(this, &SLB_SquareBox::GetFillColor)
				.Padding(0.f)
				[
					InArgs
					._Content
					.Widget
				]
			]
		]
	];
}

void SLB_SquareBox::ApplyStyle(const FLB_SquareBoxStyle& InStyle)
{
	NormalColor = InStyle.NormalColor;
	HoverColor = InStyle.HoverColor;
	PressedColor = InStyle.PressedColor;
	SelectedColor = InStyle.SelectedColor;
	HighlightColor = InStyle.HighlightColor;
	BorderColor = InStyle.BorderColor;

	// ---- 머티리얼 ----
	auto SetupMaterialBrush = [](FSlateBrush& Brush, UMaterialInterface* Material) -> bool
	{
		if (!Material)
		{
			Brush.SetResourceObject(nullptr);
			return false;
		}
		Brush.SetResourceObject(Material);
		Brush.DrawAs = ESlateBrushDrawType::Image; // 박스 전체에 늘려서 그림
		Brush.Tiling = ESlateBrushTileType::NoTile;
		return true;
	};

	bUseMaterial = InStyle.bUseMaterial;
	bTintMaterialWithStateColors = InStyle.bTintMaterialWithStateColors;
	bHasFillMaterial = SetupMaterialBrush(FillMaterialBrush, bUseMaterial ? InStyle.FillMaterial.Get() : nullptr);
	bHasBorderMaterial = SetupMaterialBrush(BorderMaterialBrush, bUseMaterial ? InStyle.BorderMaterial.Get() : nullptr);

	if (SizeBox.IsValid())
	{
		SizeBox->SetWidthOverride(FOptionalSize(static_cast<float>(InStyle.BoxSize.X)));
		SizeBox->SetHeightOverride(FOptionalSize(static_cast<float>(InStyle.BoxSize.Y)));
	}
	if (OuterBorder.IsValid())
	{
		OuterBorder->SetPadding(FMargin(InStyle.BorderThickness));
	}
}

void SLB_SquareBox::SetContent(const TSharedRef<SWidget>& InContent)
{
	if (FillBorder.IsValid())
	{
		FillBorder->SetContent(InContent);
	}
}


const FSlateBrush* SLB_SquareBox::GetFillBrush() const
{
	return bHasFillMaterial ? &FillMaterialBrush : FCoreStyle::Get().GetBrush("GenericWhiteBox");
}

const FSlateBrush* SLB_SquareBox::GetBorderBrush() const
{
	return bHasBorderMaterial ? &BorderMaterialBrush : FCoreStyle::Get().GetBrush("GenericWhiteBox");
}

FSlateColor SLB_SquareBox::GetBorderColor() const
{
	// 머티리얼 모드 + 상태색 끔 → 흰색(머티리얼 색 그대로)
	if (bHasBorderMaterial && IsMaterialTintOff()) return FLinearColor::White;
	return BorderColor;
}

FSlateColor SLB_SquareBox::GetFillColor() const
{
	if (bHasFillMaterial && IsMaterialTintOff())
		return FLinearColor::White;
	if (bHighlighted)
		return HighlightColor;
	if (bPressed)
		return PressedColor;
	if (SelectedAttr.Get(false))
		return SelectedColor;
	if (IsHovered())
		return HoverColor;
	return NormalColor;
}
