// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/LB_DMGFontManager.h"

#include "UI/LB_DamageFont.h"

// Sets default values for this component's properties





void ULB_DMGFontManager::Spawn(const FVector& WorldLocation, float Amount, bool bCrit)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController() || !WidgetClass) { return; }

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPos)) { return; }

	ULB_DamageFont* pWidget = nullptr;
	if (Pool.Num() > 0)
	{
		pWidget = Pool.Pop();
	}
	else
	{
		pWidget = CreateWidget<ULB_DamageFont>(PC, WidgetClass);
		pWidget->OnFinished.BindUObject(this, &ULB_DMGFontManager::Release);
		pWidget->AddToViewport(100);
	}

	ScreenPos.X += FMath::FRandRange(-RandomOffsetX, RandomOffsetX);
	pWidget->SetPositionInViewport(ScreenPos, true); // true = DPI 보정
	pWidget->Play(Amount, bCrit);
}
