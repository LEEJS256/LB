// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_Equipment.h"

#include "AbilitySystemComponent.h"
#include "PlayerState/LB_PlayerState.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Data/LB_DA_Equipment.h"

// Sets default values
ALB_Equipment::ALB_Equipment()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
 
	bReplicates = true;
	// 본에 부착하는 방식이라 위치 복제는 불필요 (부착 정보만 복제됨)
	SetReplicateMovement(false);
 
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 스켈레탈 메시 컴포넌트 생성 및 생성자 부착
	WeaponSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMesh"));
	WeaponSkeletalMesh->SetupAttachment(Root);
	WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 기본 충돌 끄기

	// 스태틱 메시 컴포넌트 생성 및 부착
	WeaponStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMesh"));
	WeaponStaticMesh->SetupAttachment(Root);
	WeaponStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

// void ALB_Equipment::InitializeEquipment(ULB_DA_Equipment* InItemData)
// {
// 	ItemData = InItemData;
//
// 	// Owner가 설정되어 있고 서버 권한이 있다면 DataAsset의 GE 적용
// 	if (HasAuthority())
// 	{
// 		ApplyGrantedEffects();
// 	}
// }

// Called when the game starts or when spawned
void ALB_Equipment::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		ApplyGrantedEffects();
	}
}

void ALB_Equipment::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		RemoveGrantedEffects();
	}
 
	Super::EndPlay(EndPlayReason);
}

void ALB_Equipment::ApplyGrantedEffects()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}
 
	CachedASC = ASC;

	if (ItemData)
	{
		for (const TSubclassOf<UGameplayEffect>& GEClass : ItemData->Granted_GE)
		{
			if (!GEClass) continue;

			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);

			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
			if (Spec.IsValid())
			{
				ActiveEffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()));
			}
		}
	}
	
	// for (const TSubclassOf<UGameplayEffect>& GEClass : Granted_GE)
	// {
	// 	if (!GEClass)
	// 	{
	// 		continue;
	// 	}
 //
	// 	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	// 	Context.AddSourceObject(this);
 //
	// 	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
	// 	if (Spec.IsValid())
	// 	{
	// 		ActiveEffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()));
	// 	}
	// }
}

void ALB_Equipment::RemoveGrantedEffects()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		for (const FActiveGameplayEffectHandle& Handle : ActiveEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
 
	ActiveEffectHandles.Reset();
	CachedASC.Reset();
}

UAbilitySystemComponent* ALB_Equipment::GetOwnerASC() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}
 
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
	{
		return ASC;
	}
 
	// ASC가 PlayerState에 있고 캐릭터가 IAbilitySystemInterface를 구현하지 않은 경우
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn->GetPlayerState());
	}
 
	return nullptr;
}



