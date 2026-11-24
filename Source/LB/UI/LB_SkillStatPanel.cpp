// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_SkillStatPanel.h"

#include "LB_SkillDetailPanel.h"
#include "LB_SkillPanel.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "SlateWidget/LB_Button.h"


FReply ULB_SkillStatPanel::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && DragHandle
		&& DragHandle->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDragging = true;
		DragStartScreenPos = InMouseEvent.GetScreenSpacePosition();
		DragStartOffset = PanelOffset;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULB_SkillStatPanel::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDragging)
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	// 스크린 좌표 이동량을 뷰포트 스케일로 나눠서 위젯 좌표로 (DPI 반영)
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	const FVector2D Delta = (InMouseEvent.GetScreenSpacePosition() - DragStartScreenPos) / Scale;
	const FVector2D Target = DragStartOffset + Delta;

	// 화면 밖으로 완전히 사라지지 않을 정도로만 제한
	const FVector2D VP = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
	PanelOffset = FVector2D(
		FMath::Clamp(Target.X, -VP.X + 100.f, VP.X - 100.f),
		FMath::Clamp(Target.Y, -VP.Y + 50.f,  VP.Y - 50.f));

	// 이 위젯 전체를 옮기니까 안에 있는 DetailPanel도 같이 이동
	SetRenderTranslation(PanelOffset);
	return FReply::Handled();
}

FReply ULB_SkillStatPanel::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULB_SkillStatPanel::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDragging = false; 
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void ULB_SkillStatPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this,&ULB_SkillStatPanel::HandleCloseClicked);
	}
	if (SkillPanel)
		SkillPanel->OnSkillSelected.AddUniqueDynamic(this, &ULB_SkillStatPanel::HandleSkillSelected);

	if (DetailPanel)
		DetailPanel->HidePanel();   // 스킬을 고르기 전엔 닫힌 상태
}

void ULB_SkillStatPanel::HandleCloseClicked()
{
	if (DetailPanel)
		DetailPanel->HidePanel();
	
	OnCloseRequested_SkillStat.Broadcast();
}

void ULB_SkillStatPanel::HandleSkillSelected(FName SkillCode)
{
	if (DetailPanel)
		DetailPanel->ShowSkill(SkillCode);
}

