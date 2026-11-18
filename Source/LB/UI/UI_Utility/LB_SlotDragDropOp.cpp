#include "UI/UI_Utility/LB_SlotDragDropOp.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

// ---------- DragDropOp ----------
TSharedRef<FLB_SlotDragDropOp> FLB_SlotDragDropOp::Create(FName InGroup, int32 InIndex, const FSlateBrush* InIcon)
{
	TSharedRef<FLB_SlotDragDropOp> Op = MakeShared<FLB_SlotDragDropOp>();
	Op->SourceGroup = InGroup;
	Op->SourceIndex = InIndex;
	if (InIcon)
	{
		Op->IconBrush = *InIcon;
	}
	Op->Construct();
	return Op;
}

TSharedPtr<SWidget> FLB_SlotDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBox)
		.WidthOverride(48.f)
		.HeightOverride(48.f)
		[
			SNew(SImage)
			.Image(&IconBrush)
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.7f))
		];
}
