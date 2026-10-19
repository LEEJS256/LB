// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Utility/LB_NativeGameplayTag.h"
#include "GameFramework/PlayerState.h"
#include "LB_PlayerState.generated.h"


class UAbilitySystemComponent;
class UAttributeSet;
class ULB_AttributeSet;
class ULB_PlayerDataAsset;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class LB_API ALB_PlayerState : public APlayerState , public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ALB_PlayerState();

#pragma region GAS
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	ULB_AttributeSet* GetAttributeSet() const { return AttributeSet; }

	// 서버 전용, 1회만 실행 (GasComponent가 호출)
	void InitializePlayerDA();

	// 상태 태그 교체 (GE 방식, 서버 전용)

	void AddStateTag(FGameplayTag Tag);
	void RemoveStateTag(FGameplayTag Tag);
	
	
	void GrantDefaultGA(const ULB_PlayerDataAsset* Data);
	void ApplyDefaultAttributes(const ULB_PlayerDataAsset* Data);

	// [변경] Replicated 제거 (등록 안 된 복제 프로퍼티였음)
	UPROPERTY(EditDefaultsOnly, Category = "LB|GAS")
	TSoftObjectPtr<ULB_PlayerDataAsset> CharacterData;

	// 상태 태그 → 해당 태그를 부여하는 Infinite GE
	UPROPERTY(EditDefaultsOnly, Category = "LB|GAS")
	TMap<FGameplayTag, TSubclassOf<UGameplayEffect>> StateTagToGE;

	UPROPERTY(VisibleAnywhere, Category = "LB|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<ULB_AttributeSet> AttributeSet;

#pragma endregion  

private:
	FActiveGameplayEffectHandle CurrentStateGEHandle;

	// [변경] 리스폰 때 어빌리티/GE가 중복 적용되지 않게 하는 가드
	bool bStartupApplied = false;
};
