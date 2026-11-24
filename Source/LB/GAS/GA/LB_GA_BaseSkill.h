#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/LB_GameplayAbility.h"
#include "LB_GA_BaseSkill.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 모든 스킬 GA의 부모.
 * - 발동: 코스트/쿨타임 커밋 → 몽타주 재생 → 끝나면 종료
 * - 피해량 배율은 DA_SkillInfo의 DamagePercentPerLevel을 SkillCode로 조회 (스킬 레벨 = GetAbilityLevel())
 * - 판정(근접/투사체/범위)은 자식이 OnSkillActivated / 노티파이 이벤트에서 구현
 * Cost/Cooldown GE는 BP Class Defaults에서 지정 (상세창이 여기서 수치를 읽는다)
 */
UCLASS(Abstract)
class LB_API ULB_GA_BaseSkill : public ULB_GameplayAbility
{
	GENERATED_BODY()

public:
	ULB_GA_BaseSkill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

protected:
	/** 마스터 스킬 DT의 RowName. 피해량 배율 조회용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Skill")
	FName SkillCode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Skill")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Skill", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.f;

	/** Data.Damage SetByCaller로 Health를 깎는 Instant GE */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Skill")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** 몽타주 없이 즉시 끝나는 스킬(버프 등)은 true. OnSkillActivated 안에서 직접 EndAbility 호출 */
	UPROPERTY(EditDefaultsOnly, Category = "LB|Skill")
	bool bEndManually = false;

	/** 자식 구현 지점: 커밋/몽타주 시작 직후 호출 (판정 바인딩, 이펙트 등) */
	virtual void OnSkillActivated(const FGameplayAbilityActorInfo* ActorInfo) {}

	/** 현재 스킬 레벨 (SkillComponent가 부여한 스펙 레벨) */
	UFUNCTION(BlueprintPure, Category = "LB|Skill")
	int32 GetSkillLevel() const { return FMath::Max(GetAbilityLevel(), 1); }

	/** DA의 레벨별 피해량 배율(%)을 1.0 단위 배율로 반환 (120% → 1.2). 데이터 없으면 1.0 */
	UFUNCTION(BlueprintPure, Category = "LB|Skill")
	float GetDamageMultiplier() const;

	/** 대상 ASC에 피해 GE 적용: 최종 피해 = 시전자 Attack × 배율 (서버에서만) */
	void ApplyDamageTo(AActor* Target);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();
};