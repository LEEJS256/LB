// Fill out your copyright notice in the Description page of Project Settings.

#include "Object/Destructible/LB_DestructibleProp.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

ALB_DestructibleProp::ALB_DestructibleProp()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	HitReceiver = CreateDefaultSubobject<UBoxComponent>(TEXT("HitReceiver"));
	HitReceiver->SetupAttachment(Root);
	HitReceiver->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitReceiver->SetCollisionObjectType(ECC_WorldDynamic);
	HitReceiver->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitReceiver->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitReceiver->SetGenerateOverlapEvents(true);
	HitReceiver->SetCanEverAffectNavigation(false);
}

void ALB_DestructibleProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALB_DestructibleProp, BreakState);
}

bool ALB_DestructibleProp::TryBreak(AActor* DamageCauser, const FVector& ImpactPoint, const FVector& ImpactDirection)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(DamageCauser) || DamageCauser == this)
	{
		return false;
	}

	if (BreakState.bBroken)
	{
		return false;
	}

	// 파괴 가드 확정 (상태 하나를 원자적으로 갱신)
	FLB_PropBreakState NewState;
	NewState.bBroken = true;
	NewState.ImpactPoint = ImpactPoint;
	NewState.ImpactDirection = ImpactDirection.GetSafeNormal();
	BreakState = NewState;

	// 수명 확정 (파괴 직후 즉시 Destroy 하지 않음)
	SetLifeSpan(FMath::Max(BreakLifetime, 1.f));

	ForceNetUpdate();

	// 서버 측 충돌 비활성화 및 (Listen Server라면) 표현
	ApplyBreakState();

	return true;
}

void ALB_DestructibleProp::OnRep_BreakState()
{
	ApplyBreakState();
}

void ALB_DestructibleProp::ApplyBreakState()
{
	if (!BreakState.bBroken || bBreakApplied)
	{
		return;
	}
	bBreakApplied = true;

	if (HitReceiver)
	{
		HitReceiver->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	OnBreakPresentation(BreakState.ImpactPoint, BreakState.ImpactDirection);
}
