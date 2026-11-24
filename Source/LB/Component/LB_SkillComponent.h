// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "Utility/LB_SkillEntry.h"
#include "LB_SkillComponent.generated.h"

class UAbilitySystemComponent;
class ULB_DA_SkillInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLB_OnSkillUpdated);

/**
 * 스킬 보유 컴포넌트 (PlayerState 소유)
 * - 스킬 정의(이름/아이콘/GA/MaxLevel)는 마스터 스킬 DT + ULB_DA_SkillInfo
 * - 여기는 플레이어별 상태만: 배운 스킬과 레벨, 스킬 포인트
 * - 스킬 레벨 = ASC에 부여한 GA 스펙 레벨 (GA 안에서 GetAbilityLevel()로 수치 조회)
 * - 기본 동작 GA(콤보/대시/스프린트)는 PlayerDataAsset의 StartupGA가 담당, 여기서 관리 안 함
 *
 * public
 *   1. 생명주기 / 이벤트
 *   2. 조회        : UI가 읽는 함수 (const)
 *   3. 스킬 조작    : 서버 전용
 *   4. RPC         : UI → 서버 요청
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LB_API ULB_SkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// ==========================================
	// 1. 생명주기 / 이벤트
	// ==========================================

	ULB_SkillComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 스킬 레벨이나 SP가 바뀌면 브로드캐스트 (SkillPanel / 상세창 / 퀵슬롯 갱신용) */
	UPROPERTY(BlueprintAssignable, Category = "Skill")
	FLB_OnSkillUpdated OnSkillUpdated;

	// ==========================================
	// 2. 조회
	// ==========================================

	UFUNCTION(BlueprintPure, Category = "Skill")
	const TArray<FName>& GetAvailableSkills() const { return AvailableSkills; }
	
	/** 0이면 안 배운 스킬 */
	UFUNCTION(BlueprintPure, Category = "Skill")
	int32 GetSkillLevel(FName SkillCode) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	bool IsSkillLearned(FName SkillCode) const { return GetSkillLevel(SkillCode) > 0; }

	/** SP가 있고 MaxLevel 미만인지. UI의 레벨업 버튼 활성화와 서버 검증에 같이 쓴다 */
	UFUNCTION(BlueprintPure, Category = "Skill")
	bool CanLevelUp(FName SkillCode) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	int32 GetSkillPoints() const { return SkillPoints; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	const TArray<FLB_SkillEntry>& GetLearnedSkills() const { return LearnedSkills; }

	// ==========================================
	// 3. 스킬 조작 (서버 전용)
	// ==========================================

	void InitAvailableSkills(const TArray<FName>& InSkills);
	
	/** SP 1 차감 → 레벨 +1 → GA 부여(처음) 또는 스펙 레벨 갱신. 0 → 1이 곧 "배우기" */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool LevelUpSkill(FName SkillCode);

	/** 캐릭터 레벨업 등에서 호출 */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void AddSkillPoints(int32 Count);

	// ==========================================
	// 4. RPC (UI → 서버)
	// ==========================================

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Skill")
	void Server_LevelUpSkill(FName SkillCode);

protected:
	// ==========================================
	// 복제 데이터 (본인에게만)
	// ==========================================

	UPROPERTY(ReplicatedUsing = OnRep_LearnedSkills, VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	TArray<FLB_SkillEntry> LearnedSkills;

	/** 남은 스킬 포인트 (기본값 = 시작 SP, 에디터에서) */
	UPROPERTY(ReplicatedUsing = OnRep_SkillPoints, EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	int32 SkillPoints = 0;

	//배울스킬목록
	UPROPERTY(ReplicatedUsing = OnRep_AvailableSkills, BlueprintReadOnly, Category = "Skill")
	TArray<FName> AvailableSkills;

	// ==========================================
	// OnRep
	// ==========================================

	UFUNCTION()
	void OnRep_AvailableSkills();
	
	UFUNCTION()
	void OnRep_LearnedSkills();

	UFUNCTION()
	void OnRep_SkillPoints();

private:
	// ---- 내부 헬퍼 ----
	const ULB_DA_SkillInfo* GetSkillInfo(FName SkillCode) const;
	UAbilitySystemComponent* GetASC() const;
	int32 FindSkillIndex(FName SkillCode) const;

	/** 처음이면 GiveAbility, 이미 있으면 스펙 레벨만 갱신 */
	void GrantOrUpdateAbility(FName SkillCode, const ULB_DA_SkillInfo* Info, int32 Level);
	void NotifySkillChanged();

	// ---- 서버 전용 런타임 데이터 (복제 안 함) ----
	/** 스킬 코드 → ASC에 부여한 GA 스펙 핸들 */
	TMap<FName, FGameplayAbilitySpecHandle> SkillSpecHandles;
};