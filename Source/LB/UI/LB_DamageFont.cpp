// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_DamageFont.h"
#include "Components/TextBlock.h"

void ULB_DamageFont::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	FWidgetAnimationDynamicEvent Finished;
	Finished.BindDynamic(this, &ULB_DamageFont::HandleAnimFinished);
	BindToAnimationFinished(PopAnim, Finished);
}

void ULB_DamageFont::Play(float Amount, bool bCrit)
{
	DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Amount)));
	DamageText->SetColorAndOpacity(bCrit ? CritColor : NormalColor);
	SetRenderScale(bCrit ? FVector2D(1.4f) : FVector2D(1.f));
	SetVisibility(ESlateVisibility::HitTestInvisible);
	PlayAnimation(PopAnim);
}

void ULB_DamageFont::HandleAnimFinished()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnFinished.ExecuteIfBound(this);   // 풀로 반환
}