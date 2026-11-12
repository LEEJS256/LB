#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/LB_DA_SkillInfo.h"
#include "LB_DT_MasterSkill.generated.h"

/** 마스터 스킬 테이블 행. 로우 이름 = 스킬 코드 (FLB_DT_MasterItem과 같은 방식) */
USTRUCT(BlueprintType)
struct LB_API FLB_DT_MasterSkill : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<ULB_DA_SkillInfo> SkillInfo;
};
