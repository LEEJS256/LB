#pragma once

#include "CoreMinimal.h"
#include "SLB_SquareBox.h"

/** 클릭하면 OnClicked 발생. 눌린 채로 박스 밖에서 떼면 취소. */
class LB_API SLB_Button : public SLB_SquareBox
{
public:
	SLATE_BEGIN_ARGS(SLB_Button)
		: _BoxSize(FVector2D(120.f, 40.f))
	{}
	SLATE_ARGUMENT(FVector2D, BoxSize)
	SLATE_EVENT(FOnClicked, OnClicked)
	SLATE_DEFAULT_SLOT(FArguments, Content)
SLATE_END_ARGS()

void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	FOnClicked OnClicked;
};