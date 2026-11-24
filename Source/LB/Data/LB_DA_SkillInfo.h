#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LB_DA_SkillInfo.generated.h"

class UGameplayAbility;
class UPaperSprite;

/**
 * 스킬 한 개의 정보. 마스터 스킬 테이블(FLB_DT_MasterSkill)의 로우 이름 = 스킬 코드가 이 DA를 가리킨다.
 * 쿨타임 수치는 여기 두지 않고 GA(Cooldown GE)에서 읽는다.
 */
UCLASS(BlueprintType)
class LB_API ULB_DA_SkillInfo : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<UPaperSprite> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill", meta = (MultiLine = "true"))
	FText Description;

	//참이면 패시브
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	bool bIsPassive = false;
	
	/** 이 스킬을 실행하는 GA. ASC에 부여(Grant)된 같은 클래스를 찾아 사용/쿨타임 조회에 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** 스킬 레벨 상한 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Level", meta = (ClampMin = "1"))
	int32 MaxLevel = 5;

	/** 레벨별 설명 (0번 = Lv.1). 해당 레벨 항목이 없으면 Description을 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Level", meta = (MultiLine = "true"))
	TArray<FText> LevelDescriptions;

	/** 레벨별 피해량 배율(%) (0번 = Lv.1). 비면 "-" 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Level")
	TArray<float> DamagePercentPerLevel;

	/** 해당 레벨을 배우는 데 필요한 스킬 포인트 (0번 = Lv.1 습득 비용). 비면 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Level")
	TArray<int32> PointCostPerLevel;
	
	FText GetDescriptionForLevel(int32 Level) const
	{
		const int32 Index = FMath::Max(Level, 1) - 1;
		return LevelDescriptions.IsValidIndex(Index) ? LevelDescriptions[Index] : Description;
	}
	bool GetDamagePercent(int32 Level, float& Out) const
	{
		const int32 I = FMath::Max(Level, 1) - 1;
		if (!DamagePercentPerLevel.IsValidIndex(I)) return false;
		Out = DamagePercentPerLevel[I];
		return true;
	}

	int32 GetPointCost(int32 TargetLevel) const
	{
		const int32 I = FMath::Max(TargetLevel, 1) - 1;
		return PointCostPerLevel.IsValidIndex(I) ? FMath::Max(PointCostPerLevel[I], 1) : 1;
	}
	
};