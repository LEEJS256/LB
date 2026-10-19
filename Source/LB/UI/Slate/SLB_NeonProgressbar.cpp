// Fill out your copyright notice in the Description page of Project Settings.


#include "SLB_NeonProgressbar.h"
#include "SlateOptMacros.h"
#include "Runtime/SlateCore/Public/Styling/CoreStyle.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLB_NeonProgressbar::Construct(const FArguments& InArgs)
{
	Percent = InArgs._Percent;
	BarColor = InArgs._BarColor;
	TrackColor = InArgs._TrackColor;
	GlowIntensity = InArgs._GlowIntensity;
	SlantOffset = InArgs._SlantOffset;
	IsBarVisible = InArgs._IsBarVisible;

	WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SLB_NeonProgressbar::SetPercent(TAttribute<float> InPercent)
{
	Percent = InPercent;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLB_NeonProgressbar::SetBarColor(TAttribute<FSlateColor> InBarColor)
{
	BarColor = InBarColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLB_NeonProgressbar::SetTrackColor(TAttribute<FSlateColor> InTrackColor)
{
	TrackColor = InTrackColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLB_NeonProgressbar::SetGlowIntensity(TAttribute<float> InGlowIntensity)
{
	GlowIntensity = InGlowIntensity;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLB_NeonProgressbar::SetSlantOffset(TAttribute<float> InSlantOffset)
{
	SlantOffset = InSlantOffset;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLB_NeonProgressbar::SetIsBarVisible(TAttribute<bool> InIsBarVisible)
{
	IsBarVisible = InIsBarVisible;
	Invalidate(EInvalidateWidgetReason::Paint);
}


int32 SLB_NeonProgressbar::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{

	if (!IsBarVisible.Get(true))
	{
		return LayerId;
	}
	
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    
	
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return LayerId;
	}

	const float CurrentPercent = FMath::Clamp(Percent.Get(), 0.0f, 1.0f);
	const FLinearColor BaseBarColor = BarColor.Get().GetColor(InWidgetStyle);
	const FLinearColor TintTrackColor = TrackColor.Get().GetColor(InWidgetStyle);


	const float CurrentSlant = SlantOffset.Get();
	const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();

	
	const FSlateBrush* SafeBrush = WhiteBrush ? WhiteBrush : FCoreStyle::Get().GetBrush("WhiteBrush");
	const FSlateResourceHandle& BrushResource = SafeBrush ? SafeBrush->GetRenderingResource() : FSlateResourceHandle();

	// 1. 트랙 배경
	TArray<FSlateVertex> TrackVertices;
	TArray<SlateIndex> TrackIndices;
	const FColor TrackColor8 = TintTrackColor.ToFColor(true);

	TrackVertices.Add(
		FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(0.0f, 0.0f),
		                                                   FVector2f(0.0f, 0.0f), TrackColor8));
	TrackVertices.Add(
		FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LocalSize.X, 0.0f),
		                                                   FVector2f(1.0f, 0.0f), TrackColor8));
	TrackVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
		RenderTransform, FVector2f(LocalSize.X - CurrentSlant, LocalSize.Y), FVector2f(1.0f, 1.0f), TrackColor8));
	TrackVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
		RenderTransform, FVector2f(-CurrentSlant, LocalSize.Y), FVector2f(0.0f, 1.0f), TrackColor8));

	TrackIndices.Add(0);
	TrackIndices.Add(1);
	TrackIndices.Add(2);
	TrackIndices.Add(0);
	TrackIndices.Add(2);
	TrackIndices.Add(3);

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements, LayerId, BrushResource, TrackVertices, TrackIndices, nullptr, 0, 0
	);


// 2. 게이지 채우기 (고밀도 네온 슬릿 세그먼트)

	const bool bBarVisible = IsBarVisible.Get(true);
	
    if (bBarVisible && CurrentPercent > 0.0f)
    {
        const float FillWidth = LocalSize.X * CurrentPercent;
        
        // ⭐ 시안 비율: 8px 채움 + 1.5px 얇은 틈
        const float BlockWidth = 7.0f;   // 발광 바 너비
        const float BlockGap = 1.5f;     // 미세한 분할선 두께
        const float Step = BlockWidth + BlockGap;

        const FSlateBrush* SlateSafeBrush = WhiteBrush ? WhiteBrush : FCoreStyle::Get().GetBrush("WhiteBrush");

        for (float CurrentX = 0.0f; CurrentX + 2.0f <= FillWidth; CurrentX += Step)
        {
            // 마지막 조각이 게이지 끝단을 넘어가지 않도록 클램핑
            const float ActualBlockWidth = FMath::Min(BlockWidth, FillWidth - CurrentX);

            // 좌측에서 우측으로 갈수록 밝아지는 네온 그라데이션
            const float Ratio = FMath::Clamp((CurrentX + ActualBlockWidth) / LocalSize.X, 0.0f, 1.0f);
            const FLinearColor BlockColor = FMath::Lerp(BaseBarColor * 0.55f, BaseBarColor * (GlowIntensity.Get() * 1.1f), Ratio);
            const FColor FinalColor = BlockColor.ToFColor(true);

            TArray<FSlateVertex> BlockVerts;
            TArray<SlateIndex> BlockIndices;

            // 전체 바의 기울기와 동일한 각도로 평행사변형 조각 생성
            BlockVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
                RenderTransform, FVector2f(CurrentX, 0.0f), FVector2f(0.0f, 0.0f), FinalColor));
            BlockVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
                RenderTransform, FVector2f(CurrentX + ActualBlockWidth, 0.0f), FVector2f(1.0f, 0.0f), FinalColor));
            BlockVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
                RenderTransform, FVector2f(CurrentX + ActualBlockWidth - CurrentSlant, LocalSize.Y), FVector2f(1.0f, 1.0f), FinalColor));
            BlockVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
                RenderTransform, FVector2f(CurrentX - CurrentSlant, LocalSize.Y), FVector2f(0.0f, 1.0f), FinalColor));

            BlockIndices.Add(0); BlockIndices.Add(1); BlockIndices.Add(2);
            BlockIndices.Add(0); BlockIndices.Add(2); BlockIndices.Add(3);

            FSlateDrawElement::MakeCustomVerts(
                OutDrawElements,
                LayerId + 1,
                SlateSafeBrush->GetRenderingResource(),
                BlockVerts,
                BlockIndices,
                nullptr, 0, 0
            );
        }

        // 게이지 끝단 엣지 하이라이트 (시안처럼 더 선명한 흰색 캡)
        TArray<FVector2f> CapPoints;
        CapPoints.Add(FVector2f(FillWidth, 0.0f));
        CapPoints.Add(FVector2f(FillWidth - CurrentSlant, LocalSize.Y));

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            CapPoints,
            ESlateDrawEffect::None,
            FLinearColor(1.5f, 1.5f, 1.5f, 1.0f), // 오버드라이브 화이트
            true,
            2.0f
        );
    }
	// 3. 다중 외곽 글로우
	const int32 GlowSteps = 2;
	for (int32 i = GlowSteps; i >= 1; --i)
	{
		float Spread = (float)i * 1.5f;
		float Alpha = 0.25f / (float)i;

		TArray<FVector2f> SlantedGlow;
		SlantedGlow.Add(FVector2f(-Spread, -Spread));
		SlantedGlow.Add(FVector2f(LocalSize.X + Spread, -Spread));
		SlantedGlow.Add(FVector2f(LocalSize.X - CurrentSlant + Spread, LocalSize.Y + Spread));
		SlantedGlow.Add(FVector2f(-CurrentSlant - Spread, LocalSize.Y + Spread));
		SlantedGlow.Add(FVector2f(-Spread, -Spread));

		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), SlantedGlow, ESlateDrawEffect::None,
			FLinearColor(BaseBarColor.R, BaseBarColor.G, BaseBarColor.B, Alpha), true, 1.5f + (float)i * 1.2f
		);
	}

	// 4. 외곽 테두리
	TArray<FVector2f> SharpFrame;
	SharpFrame.Add(FVector2f(0.0f, 0.0f));
	SharpFrame.Add(FVector2f(LocalSize.X, 0.0f));
	SharpFrame.Add(FVector2f(LocalSize.X - CurrentSlant, LocalSize.Y));
	SharpFrame.Add(FVector2f(-CurrentSlant, LocalSize.Y));
	SharpFrame.Add(FVector2f(0.0f, 0.0f));

	FSlateDrawElement::MakeLines(
		OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), SharpFrame, ESlateDrawEffect::None,
		BaseBarColor, true, 1.2f
	);

	return LayerId + 5;
}


FVector2D SLB_NeonProgressbar::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(100.0f, 20.0f);
}
