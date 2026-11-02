// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_Dodge.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Components/CapsuleComponent.h"
#include "GAS/Attribute/LB_AttributeSet.h"

ULB_GA_Dodge::ULB_GA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void ULB_GA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                   const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Character || !ASC || !DodgeMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 어트리뷰트 읽기 (활성화 시점에 캐싱)
	float DashDistance = FallbackDashDistance;
	float DashDuration = FallbackDashDuration;
	
    bool bFound = false;
    const float DistValue = ASC->GetGameplayAttributeValue(ULB_AttributeSet::GetDashDistanceAttribute(), bFound);
    if (bFound && DistValue > 0.f) DashDistance = DistValue;

    bFound = false;
    const float DurValue = ASC->GetGameplayAttributeValue(ULB_AttributeSet::GetDashDurationAttribute(), bFound);
    if (bFound && DurValue > KINDA_SMALL_NUMBER) DashDuration = DurValue;

    // 2.방향 결정: 입력 > 현재 속도 > 메시 방향 순서
	FVector Dir = Character->GetLastMovementInputVector();
	if (Dir.IsNearlyZero())
	{
		Dir = Character->GetVelocity();
	}
	if (Dir.IsNearlyZero())
	{
		// 입력도 속도도 없으면 메시가 보는 방향 (메시 Forward 축은 프로젝트에 맞게)
		Dir = Character->GetMesh()->GetRightVector(); // 보통 메시가 -90도 돌아가 있어서 Right/Forward 중 하나
	}
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();

    // 3. 벽 클램프
    const FVector Start = Character->GetActorLocation();
    FVector End = Start + Dir * DashDistance;

    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DodgeSweep), false, Character);
    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params))
    {
        End = Start + Dir * FMath::Max(0.f, Hit.Distance - 1.f);
    }

    // 4. 이동 (CMC RootMotionSource, 예측/보정 지원)
	UAbilityTask_ApplyRootMotionMoveToForce* MoveTask =
	UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,                                   // OwningAbility
		NAME_None,                              // TaskInstanceName
		End,                                    // TargetLocation
		DashDuration,                           // Duration
		false,                                  // bSetNewMovementMode
		MOVE_Walking,                           // MovementMode
		true,                                   // bRestrictSpeedToExpected
		nullptr,                                // PathOffsetCurve
		ERootMotionFinishVelocityMode::SetVelocity, // VelocityOnFinishMode
		FVector::ZeroVector,                    // SetVelocityOnFinish
		0.f);                                   // ClampVelocityOnFinish
	MoveTask->ReadyForActivation();

    // 5. 몽타주 길이를 이동 시간에 맞춤
    const float PlayRate = DodgeMontage->GetPlayLength() / DashDuration;
    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DodgeMontage, PlayRate);
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
    MontageTask->ReadyForActivation();

    // 6. 노티파이 이벤트 대기
    UAbilityTask_WaitGameplayEvent* WaitTask =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EndEventTag);
    WaitTask->EventReceived.AddDynamic(this, &ThisClass::OnEndEvent);
    WaitTask->ReadyForActivation();
}

void ULB_GA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                              bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULB_GA_Dodge::OnMontageFinished()
{
 	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULB_GA_Dodge::OnEndEvent(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
