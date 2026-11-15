// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/LB_DMGFontManager.h"
#include "UI/LB_DamageFont.h"


ULB_DMGFontManager::ULB_DMGFontManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

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
		pWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f)); // 숫자 중앙 기준
		pWidget->AddToViewport(100);
	}
	Active.Add({
		pWidget, WorldLocation,
		FVector2D(FMath::FRandRange(-RandomOffsetX, RandomOffsetX), 0.f)
	});


	pWidget->Play(Amount, bCrit);
	SetComponentTickEnabled(true);
}

void ULB_DMGFontManager::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
		{ return; }

	for (const FActiveEntry& E : Active)
	{
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(E.WorldLocation, ScreenPos, true))
		{
			E.Widget->SetPositionInViewport(ScreenPos + E.ScreenOffset, true); // 위에서 DPI 보정함
			E.Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			E.Widget->SetVisibility(ESlateVisibility::Hidden); // 카메라 뒤/화면 밖
		}
	}
}

void ULB_DMGFontManager::Release(ULB_DamageFont* Widget)
{
	Active.RemoveAll([Widget](const FActiveEntry& E) { return E.Widget == Widget; });
	Pool.Add(Widget);
	if (Active.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}
