#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LB_DA_SkillInfo.generated.h"

class UGameplayAbility;
class UTexture2D;

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
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill", meta = (MultiLine = "true"))
	FText Description;

	/** 이 스킬을 실행하는 GA. ASC에 부여(Grant)된 같은 클래스를 찾아 사용/쿨타임 조회에 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** 스킬 레벨 상한 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Level", meta = (ClampMin = "1"))
	int32 MaxLevel = 5;

	/** 레벨별 설명 (0번 = Lv.1). 해당 레벨 항목이 없으면 Description을 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Level", meta = (MultiLine = "true"))
	TArray<FText> LevelDescriptions;

	FText GetDescriptionForLevel(int32 Level) const
	{
		const int32 Index = FMath::Max(Level, 1) - 1;
		return LevelDescriptions.IsValidIndex(Index) ? LevelDescriptions[Index] : Description;
	}
};