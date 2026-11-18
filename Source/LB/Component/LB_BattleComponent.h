// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/LB_DT_ComboInfo.h"
#include "LB_BattleComponent.generated.h"

class UAbilitySystemComponent;


// 플레이어 공격 요청의 공통 진입점. ASC는 소유하지 않고 Owner(IAbilitySystemInterface)가 노출하는 기존 ASC에 활성화를 요청한다
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LB_API ULB_BattleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULB_BattleComponent();

	// ATK.Normal 태그 Ability 활성화 요청
	UFUNCTION(BlueprintCallable, Category = "LB|Battle")
	bool RequestNormalAttack();

	// Input.RightClick 이벤트를 ASC OwnerActor에 전달한 뒤 ATK.Strong 태그 Ability 활성화 요청
	UFUNCTION(BlueprintCallable, Category = "LB|Battle")
	bool RequestStrongAttack();

	// 태그/ASC/Avatar 확인 후 TryActivateAbilitiesByTag 요청 결과를 반환 (서버 확정 결과가 아님)
	UFUNCTION(BlueprintCallable, Category = "LB|Battle")
	bool RequestAttackByTag(const FGameplayTag& AttackTag);

	//State.ATK사라지면 알림해주는 함수
	void BindCombatStateEvents();

#pragma region Combo
	//새로추가 좌클릭+우클릭 통합
	UFUNCTION(BlueprintCallable, Category = "LB|Battle")
	bool RequestComboInput(const FGameplayTag& InputTag);

	// GA 시작 시: 버퍼된 입력으로 첫 섹션 결정
	bool ConsumeStartSection(FName& OutSection);

	// AnimNotifyState(ComboWindow)에서 호출
	void OpenComboWindow();
	// 윈도우 종료 시 버퍼 확인 후 전이. 전이되면 true + 섹션 반환
	bool CloseComboWindow(FName& OutNextSection);

	// GA EndAbility에서 반드시 호출
	void ResetCombo();
#pragma endregion
	
#pragma  region MotionWarping
	UFUNCTION(BlueprintCallable, Category="LB|Combat")
	void UpdateAttackWarpTarget();

	UPROPERTY(EditAnywhere, Category="LB|Combat|Warp")
	float WarpSearchRadius = 200.f;   // 탐색 거리

	UPROPERTY(EditAnywhere, Category="LB|Combat|Warp")
	float WarpStopDistance = 90.f;    // 타겟 앞 멈출 거리

	UPROPERTY(EditAnywhere, Category="LB|Combat|Warp")
	float WarpMaxDistance = 150.f;    // 한 번에 땡겨지는 최대 거리

	UPROPERTY(EditAnywhere, Category="LB|Combat|Warp")
	float WarpFacingDot = 0.3f;       // 앞쪽 판정 (뒤에 있는 적 무시)

	static const FName WarpTargetName;
	
#pragma  endregion 


protected:
	virtual void BeginPlay() override;
	
	// Owner가 IAbilitySystemInterface로 노출하는 ASC를 매번 조회 (GAS 초기화 전에는 nullptr)
	UAbilitySystemComponent* ResolveAbilitySystemComponent() const;

	UPROPERTY(EditDefaultsOnly, Category = "LB|Combo")
	TObjectPtr<UDataTable> ComboTable;
	
private:
	// ASC가 있고 Avatar가 이 컴포넌트의 Owner인지 확인
	bool CanRequestAttack(const UAbilitySystemComponent* ASC) const;

	// 태그 사라졌을때 실행됨
	void OnATKTagChanged(const FGameplayTag Tag, int32 NewCount);
	// 공격 중인지 체크
	bool bATKBound = false;


	bool TryAdvance(const FGameplayTag& InputTag, FName& OutSection);

	TMap<TPair<FGameplayTag, FGameplayTag>, FLB_DT_ComboInfo> ComboMap;
	FGameplayTag CurrentComboState;
	FGameplayTag BufferedInput;
	bool bComboWindowOpen = false;


};
