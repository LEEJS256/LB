// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "LB_AttributeSet.generated.h"


// 매크로 - Getter/Setter 자동 생성
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class LB_API ULB_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	ULB_AttributeSet();

	void Set_PlayerName(FName ArgName);
	FName Get_PlayerName() const { return PlayerName; }

#pragma region AttributeSet
	// 모든 Attribute들의 이전값

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_PlayerName)
	FName PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Speed)
	FGameplayAttributeData Speed;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Speed)

	//DashDistance
	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_DashDistance)
	FGameplayAttributeData DashDistance;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, DashDistance)

	//DashDuration
	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_DashDuration)
	FGameplayAttributeData DashDuration;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, DashDuration)
	

	//달리기 가중치
	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_SprintWeight)
	FGameplayAttributeData SprintWeight;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, SprintWeight)

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, MaxHealth)

	// Stamina
	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, MaxStamina)

	// Experience & Level
	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Exp)
	FGameplayAttributeData Exp;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Exp)

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_MaxExp)
	FGameplayAttributeData MaxExp;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, MaxExp)

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Level)
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Level)

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Gold)
	FGameplayAttributeData Gold;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Gold)

	// Combat Stats
	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Attack)
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Attack)

	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Armor)

	//강인도
	UPROPERTY(BlueprintReadOnly, Category = "Attributeset", ReplicatedUsing = OnRep_Poise)
	FGameplayAttributeData Poise;
	ATTRIBUTE_ACCESSORS(ULB_AttributeSet, Poise)


#pragma endregion

#pragma region Delegate


	//데미지용
	// DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDamageTaken, float, FVector);
	DECLARE_MULTICAST_DELEGATE_FourParams(
		FOnDamageTaken,
		float, // Damage
		FVector, // HitLocation
		bool, // Critical
		FGameplayTag // DamageType
	);

	FOnDamageTaken OnDamageTaken;
#pragma  endregion

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//attribute 값이 실제로 변경되기 직전
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	//attribute 값이 실제로 변경되기 직후
	void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	//GE가 실행된 직후에만 호출
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;


#pragma region Replicated
	// UFUNCTION()
	// virtual void OnRep_Name(const FGameplayAttributeData& OldName);
	UFUNCTION()
	void OnRep_PlayerName(const FName& OldName);

	UFUNCTION()
	virtual void OnRep_Speed(const FGameplayAttributeData& OldSpeed);


	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);


	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	// Stamina
	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	// Experience & Level
	UFUNCTION()
	virtual void OnRep_Exp(const FGameplayAttributeData& OldExp);

	UFUNCTION()
	virtual void OnRep_MaxExp(const FGameplayAttributeData& OldMaxExp);

	UFUNCTION()
	virtual void OnRep_Level(const FGameplayAttributeData& OldLevel);

	UFUNCTION()
	virtual void OnRep_Gold(const FGameplayAttributeData& OldGold);

	// Combat Stats
	UFUNCTION()
	virtual void OnRep_Attack(const FGameplayAttributeData& OldAttack);

	UFUNCTION()
	virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);

	UFUNCTION()
	virtual void OnRep_Poise(const FGameplayAttributeData& OldPoise);

	UFUNCTION()
	virtual void OnRep_SprintWeight(const FGameplayAttributeData& OldSprintWeight);

	UFUNCTION()
	virtual void OnRep_DashDistance(const FGameplayAttributeData& OldDashDistance);

	UFUNCTION()
	virtual void OnRep_DashDuration(const FGameplayAttributeData& OldDashDuration);


#pragma endregion
};
