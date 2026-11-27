// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h" 
#include "LB_MonsterBase.generated.h"

class UAbilitySystemComponent;
class ULB_AttributeSet;
class ULB_MonsterDataAsset;
class UWidgetComponent;
class ALB_MonsterBase;

// 서버에서 몬스터 Health가 처음 0 이하가 되었을 때 한 번 발행되는 사망 완료 신호
DECLARE_MULTICAST_DELEGATE_OneParam(FLBOnMonsterDefeated, ALB_MonsterBase* /*DefeatedMonster*/);

UCLASS()
class LB_API ALB_MonsterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALB_MonsterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	ULB_AttributeSet* GetAttributeSet() const { return AttributeSet; }

	// ASC의 State.Death 보유 여부. 서버가 부여하고 TagOnly로 복제되므로 클라이언트에서도 사용 가능
	UFUNCTION(BlueprintPure, Category = "LB|State")
	bool IsDead() const;

	// 서버 전용 사망 완료 신호 구독 API. Client에서는 발행되지 않는다
	FLBOnMonsterDefeated& GetOnMonsterDefeated() { return OnMonsterDefeated; }

	// 서버에서 사망 완료 신호가 이미 발행되었는지 여부
	bool IsDefeatNotified() const { return bDefeatNotified; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
#pragma region Monster_UI
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> HPBarComp;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarHideDelay = 3.f;

	FTimerHandle HPBarHideTimer;

	void Set_HPComp();
	void Begin_HPComp();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void HideHPBar();
#pragma  endregion

#pragma region AttributeSet
	void ApplyDefaultAttributes(const ULB_MonsterDataAsset* Data);

	// 몬스터 자체 ASC (Owner = Avatar = Monster)
	UPROPERTY(VisibleAnywhere, Category = "LB|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<ULB_AttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "LB|GAS")
	TObjectPtr<ULB_MonsterDataAsset> MonsterData;

	
#pragma endregion 
private:
	// 서버 전용, 1회만 실행
	void InitializeMonsterData();

	// 모든 World(Dedicated 포함)에서 Health 변경 구독을 한 번만 연결
	void BindHealthChanged();
	// 서버의 최초 0 이하 전이만 사망 완료로 발행
	void TryNotifyDefeated(const FOnAttributeChangeData& Data);

	FLBOnMonsterDefeated OnMonsterDefeated;

	bool bStartupApplied = false;
	bool bHealthChangeBound = false;
	bool bApplyingDefaultAttributes = false;
	bool bDefeatNotified = false;
};
