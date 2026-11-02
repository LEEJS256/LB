// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "LB_SquareBox.h"
#include "LB_Button.generated.h"

class STextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLB_OnButtonClicked);

/**
 * 클릭 가능한 네모 버튼 (UMG용).
 * - 디자이너에서 자식 위젯을 넣지 않으면 LabelText가 가운데에 표시된다.
 *   자식 위젯(이미지 등)을 넣으면 그 자식이 우선하고 라벨은 쓰이지 않는다.
 * - 테두리색/두께/배경/상태색은 부모(ULB_SquareBox)의 Style 그대로 사용.
 */
UCLASS()
class LB_API ULB_Button : public ULB_SquareBox
{
	GENERATED_BODY()

public:
	ULB_Button();

	// ---- 이벤트 ----
	UPROPERTY(BlueprintAssignable, Category = "LB|Button")
	FLB_OnButtonClicked OnClicked;

	// ---- 라벨 ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|Button|Label")
	FText LabelText;

	/** 글꼴 + 글자 크기(+스타일)를 한 번에 지정 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|Button|Label")
	FSlateFontInfo Font;

	/** 평상시 글자색 (Hex sRGB 입력 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|Button|Label")
	FColor TextColor = FColor(0, 229, 255);        // #00E5FF

	/** 마우스를 올렸을 때 글자색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|Button|Label")
	FColor HoverTextColor = FColor(230, 251, 255); // #E6FBFF

	UFUNCTION(BlueprintCallable, Category = "LB|Button")
	void SetLabelText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category = "LB|Button")
	void SetFont(const FSlateFontInfo& InFont);

	UFUNCTION(BlueprintCallable, Category = "LB|Button")
	void SetTextColor(FColor InColor);

	// ---- 테두리 (Style의 값을 런타임에 바꾸는 편의 함수) ----
	UFUNCTION(BlueprintCallable, Category = "LB|Button")
	void SetBorderColor(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "LB|Button")
	void SetBorderThickness(float InThickness);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual TSharedRef<SLB_SquareBox> CreateSquareBox() override;

private:
	FReply HandleSlateClicked();
	void ApplyLabel();

	TSharedPtr<STextBlock> MyLabel;
};