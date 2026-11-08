#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"
#include "LB_QuickSlotTextStyle.generated.h"

/** 퀵슬롯 안에 들어가는 글자(단축키 라벨 / 수량) 하나의 모양 + 위치 */
USTRUCT(BlueprintType)
struct LB_API FLB_QuickSlotTextStyle
{
	GENERATED_BODY()

	/** 비워두면(폰트 미지정) 기본 Bold 10 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FLinearColor Color = FLinearColor::White;

	/** 글자가 여러 줄/폭이 있을 때의 문단 정렬 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	TEnumAsByte<ETextJustify::Type> Justification = ETextJustify::Left;

	/** 칸 안에서의 위치 (좌상단 = Left + Top) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	TEnumAsByte<EHorizontalAlignment> HAlign = HAlign_Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	TEnumAsByte<EVerticalAlignment> VAlign = VAlign_Top;

	/** 테두리에서 떨어진 정도 (Left, Top, Right, Bottom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FMargin Padding = FMargin(4.f, 2.f, 0.f, 0.f);

	/** 단축키 라벨 기본값: 좌상단 */
	static FLB_QuickSlotTextStyle HotkeyDefault()
	{
		return FLB_QuickSlotTextStyle();
	}

	/** 수량 기본값: 우하단 */
	static FLB_QuickSlotTextStyle CountDefault()
	{
		FLB_QuickSlotTextStyle S;
		S.Justification = ETextJustify::Right;
		S.HAlign = HAlign_Right;
		S.VAlign = VAlign_Bottom;
		S.Padding = FMargin(0.f, 0.f, 4.f, 2.f);
		return S;
	}
};