// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Monster/LB_MonsterBase.h"

#include "AbilitySystemComponent.h"
#include "GAS/Attribute/LB_AttributeSet.h"

// Sets default values
ALB_MonsterBase::ALB_MonsterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// 같은 액터의 서브오브젝트라 ASC에 자동 등록됨
	AttributeSet = CreateDefaultSubobject<ULB_AttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ALB_MonsterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void ALB_MonsterBase::BeginPlay()
{
	Super::BeginPlay();

	// 서버/클라이언트 모두 Owner와 Avatar를 몬스터 자신으로 초기화
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}
