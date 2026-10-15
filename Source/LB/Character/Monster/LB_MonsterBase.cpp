// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Monster/LB_MonsterBase.h"

#include "LB.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Data/LB_MonsterDataAsset.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "Utility/LB_NativeGameplayTag.h"

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

bool ALB_MonsterBase::IsDead() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Death);
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

	InitializeMonsterData();
}

void ALB_MonsterBase::InitializeMonsterData()
{
	if (!HasAuthority() || bStartupApplied || !AbilitySystemComponent)
		return;

	if (!MonsterData)
	{
		UE_LOG(LogLB, Warning, TEXT("%s: MonsterData가 설정되지 않아 초기 GAS 데이터를 적용하지 않습니다."), *GetName());
		return;
	}

	bStartupApplied = true;

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : MonsterData->StartupGA)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	}

	FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
	Ctx.AddSourceObject(this);
	for (const TSubclassOf<UGameplayEffect>& GEClass : MonsterData->StartupGE)
	{
		if (GEClass)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(GEClass->GetDefaultObject<UGameplayEffect>(), 1.f, Ctx);
		}
	}
}
