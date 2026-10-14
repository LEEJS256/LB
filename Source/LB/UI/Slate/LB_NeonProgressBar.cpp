// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_NeonProgressBar.h"
#include "SLB_NeonProgressbar.h"

TSharedRef<SWidget> ULB_NeonProgressBar::RebuildWidget()
{
	MyProgressBar = SNew(SLB_NeonProgressbar)
		.Percent(Percent)
		.BarColor(FSlateColor(BarColor))
		.TrackColor(FSlateColor(TrackColor))
		.GlowIntensity(GlowIntensity)
		.IsBarVisible(bIsBarVisible)
		.SlantOffset(SlantOffset);

	return MyProgressBar.ToSharedRef();
}

void ULB_NeonProgressBar::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetPercent(Percent);
		MyProgressBar->SetBarColor(FSlateColor(BarColor));
		MyProgressBar->SetGlowIntensity(GlowIntensity);
		MyProgressBar->SetSlantOffset(SlantOffset);
		MyProgressBar->SetIsBarVisible(bIsBarVisible);
	}
}

void ULB_NeonProgressBar::SetBarVisibility(bool bInIsVisible)
{
	bIsBarVisible = bInIsVisible;
	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetIsBarVisible(bIsBarVisible);
	}
}

void ULB_NeonProgressBar::SetPercent(float InPercent)
{
	Percent = FMath::Clamp(InPercent, 0.0f, 1.0f);
	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetPercent(Percent);
	}
}

void ULB_NeonProgressBar::SetSlentOffset(float InPercent)
{
	SlantOffset = InPercent;

	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetSlantOffset(InPercent);
	}
}

void ULB_NeonProgressBar::SetBarColor(FLinearColor InColor)
{
	BarColor = InColor;
	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetBarColor(FSlateColor(BarColor));
	}
}

void ULB_NeonProgressBar::SetTrackColor(FLinearColor InColor)
{
	TrackColor = InColor;
	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetTrackColor(FSlateColor(TrackColor));
	}
}

void ULB_NeonProgressBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyProgressBar.Reset();
}

#if WITH_EDITOR
const FText ULB_NeonProgressBar::GetPaletteCategory()
{
	return FText::FromString(TEXT("LB UI Custom"));
}
#endif
