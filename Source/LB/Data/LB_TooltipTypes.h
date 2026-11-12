// 툴팁 표시용 공용 타입 모음 (DA와 UI가 같이 사용)

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "LB_TooltipTypes.generated.h"

/** 툴팁 한 줄의 색상 종류. 실제 색은 ULB_ItemTooltip::LineColors에서 정한다. */
UENUM(BlueprintType)
enum class ELB_TooltipLineStyle : uint8
{
	Normal,     // 흰색: 기본 줄 (예: STR +10)
	Bonus,      // 초록색: 추가/강화 수치
	Highlight   // 노랑/주황: 강조 줄 (예: HP 100 회복, 중복 장착 불가)
};

/** 툴팁 한 줄. DA가 GetTooltipLines에서 채워 준다. */
USTRUCT(BlueprintType)
struct FLB_TooltipLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Tooltip")
	FText Label;                // "STR", "HP"

	UPROPERTY(BlueprintReadWrite, Category = "Tooltip")
	FText Value;                // "+10", "100 회복"

	UPROPERTY(BlueprintReadWrite, Category = "Tooltip")
	ELB_TooltipLineStyle Style = ELB_TooltipLineStyle::Normal;
};

/** 능력치 증감 한 항목. 장비의 추가 능력치, 소비템의 효과 목록에서 공용으로 사용. */
USTRUCT(BlueprintType)
struct FLB_StatBonus
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttribute Attribute;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Value = 0.f;
};
