// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_SkillStatPanel.generated.h"

/**
 * 
 */

class ULB_Button;
class ULB_SkillPanel;
class ULB_SkillDetailPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLB_OnSkillStatCloseRequested);

UCLASS()
class LB_API ULB_SkillStatPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FLB_OnSkillStatCloseRequested OnCloseRequested_SkillStat;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;


	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_Button> CloseButton;

	/** 잡고 끄는 영역 (제목줄). 없으면 드래그 안 됨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DragHandle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_SkillPanel> SkillPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_SkillDetailPanel> DetailPanel;

private:
	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleSkillSelected(FName SkillCode);

	bool bDragging = false;
	FVector2D DragStartScreenPos = FVector2D::ZeroVector;
	FVector2D DragStartOffset = FVector2D::ZeroVector;
	FVector2D PanelOffset = FVector2D::ZeroVector;
};
