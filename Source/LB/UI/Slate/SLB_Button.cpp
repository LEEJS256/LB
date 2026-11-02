#include "SLB_Button.h"

void SLB_Button::Construct(const FArguments& InArgs)
{
	OnClicked = InArgs._OnClicked;

	SLB_SquareBox::Construct(
		SLB_SquareBox::FArguments()
		.BoxSize(InArgs._BoxSize)
		[
			InArgs._Content.Widget
		]);
}

FReply SLB_Button::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetPressed(true);
		return FReply::Handled().CaptureMouse(AsShared());
	}
	return FReply::Unhandled();
}

FReply SLB_Button::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPressed)
	{
		const bool bInside = MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition());
		SetPressed(false);

		FReply Reply = FReply::Handled();
		if (bInside && OnClicked.IsBound())
		{
			Reply = OnClicked.Execute();
		}
		return Reply.ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}