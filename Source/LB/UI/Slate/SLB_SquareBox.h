#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "FLB_SquareBoxStyle.h"
#include "Components/Widget.h"

class SBox;
class SBorder;

/**
 * 베이스 네모박스: 테두리 + 배경 + 상태색만 담당.
 * 상태 우선순위: Highlight(드래그 오버) > Pressed > Selected > Hover > Normal
 */
class LB_API SLB_SquareBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLB_SquareBox)
		: _BoxSize(FVector2D(64.f, 64.f))
		, _BorderThickness(2.f)
		, _BorderColor(FLinearColor(0.f, 0.9f, 1.f, 1.f))
		, _NormalColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.85f))
		, _HoverColor(FLinearColor(0.05f, 0.15f, 0.2f, 0.9f))
		, _PressedColor(FLinearColor(0.f, 0.35f, 0.45f, 0.95f))
		, _SelectedColor(FLinearColor(0.f, 0.25f, 0.3f, 0.95f))
		, _HighlightColor(FLinearColor(0.f, 0.5f, 0.6f, 0.95f))
	{}
		SLATE_ARGUMENT(FVector2D, BoxSize)
		SLATE_ARGUMENT(float, BorderThickness)
		SLATE_ARGUMENT(FLinearColor, BorderColor)
		SLATE_ARGUMENT(FLinearColor, NormalColor)
		SLATE_ARGUMENT(FLinearColor, HoverColor)
		SLATE_ARGUMENT(FLinearColor, PressedColor)
		SLATE_ARGUMENT(FLinearColor, SelectedColor)
		SLATE_ARGUMENT(FLinearColor, HighlightColor)
		SLATE_ATTRIBUTE(bool, Selected)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** UMG 쪽에서 런타임 갱신용 */
	void ApplyStyle(const FLB_SquareBoxStyle& InStyle);
	void SetContent(const TSharedRef<SWidget>& InContent);
	void SetSelected(bool bInSelected) { SelectedAttr.Set(bInSelected); }

	
protected:
	void SetPressed(bool bInPressed) { bPressed = bInPressed; }
	void SetHighlighted(bool bInHighlighted) { bHighlighted = bInHighlighted; }

	bool bPressed = false;
	bool bHighlighted = false;

private:
	FSlateColor GetFillColor() const;

	TSharedPtr<SBox> SizeBox;
	TSharedPtr<SBorder> OuterBorder;
	TSharedPtr<SBorder> FillBorder;

	FLinearColor NormalColor;
	FLinearColor HoverColor;
	FLinearColor PressedColor;
	FLinearColor SelectedColor;
	FLinearColor HighlightColor;
	TAttribute<bool> SelectedAttr;
};