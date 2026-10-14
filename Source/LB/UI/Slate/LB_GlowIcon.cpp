#include "LB_GlowIcon.h"
#include "SLB_GlowIcon.h"

TSharedRef<SWidget> ULB_GlowIcon::RebuildWidget()
{
	MyGlowIcon = SNew(SLB_GlowIcon)
		.IconText(IconText)
		.IconColor(FSlateColor(IconColor))
		.BackgroundColor(FSlateColor(BackgroundColor))
		.GlowIntensity(GlowIntensity)
		.GlowBoxSize(BoxSize);

	return MyGlowIcon.ToSharedRef();
}

void ULB_GlowIcon::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyGlowIcon.IsValid())
	{
		MyGlowIcon->SetIconText(IconText);
		MyGlowIcon->SetIconColor(FSlateColor(IconColor));
		MyGlowIcon->SetGlowIntensity(GlowIntensity);
	}
}

void ULB_GlowIcon::SetIconText(FText InText)
{
	IconText = InText;
	if (MyGlowIcon.IsValid())
	{
		MyGlowIcon->SetIconText(IconText);
	}
}

void ULB_GlowIcon::SetIconColor(FLinearColor InColor)
{
	IconColor = InColor;
	if (MyGlowIcon.IsValid())
	{
		MyGlowIcon->SetIconColor(FSlateColor(IconColor));
	}
}

void ULB_GlowIcon::SetGlowIntensity(float InIntensity)
{
	GlowIntensity = InIntensity;
	if (MyGlowIcon.IsValid())
	{
		MyGlowIcon->SetGlowIntensity(InIntensity);
	}
}

void ULB_GlowIcon::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyGlowIcon.Reset();
}

#if WITH_EDITOR
const FText ULB_GlowIcon::GetPaletteCategory()
{
	return FText::FromString(TEXT("LB UI Custom"));
}
#endif