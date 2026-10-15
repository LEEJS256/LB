// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "LB_MonsterBase.generated.h"

class UAbilitySystemComponent;
class ULB_AttributeSet;

UCLASS()
class LB_API ALB_MonsterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALB_MonsterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	ULB_AttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 몬스터 자체 ASC (Owner = Avatar = Monster)
	UPROPERTY(VisibleAnywhere, Category = "LB|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<ULB_AttributeSet> AttributeSet;
};
