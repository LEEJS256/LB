// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/LB_MonsterAIController.h"

#include "AbilitySystemComponent.h"
#include "Character/Monster/LB_MeleeMonster.h"
#include "Character/Monster/LB_MonsterBase.h"
#include "Component/LB_MeleeHitboxComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "LB.h"
#include "StateTree.h"
#include "TimerManager.h"
#include "Utility/LB_NativeGameplayTag.h"

ALB_MonsterAIController::ALB_MonsterAIController()
{
	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
	// 시작은 TryStartCombatLogic에서만 수행
	StateTreeAIComponent->SetStartLogicAutomatically(false);
	BrainComponent = StateTreeAIComponent;

	// AAIController::OnPossess의 암묵적인 Brain 시작 비활성화
	bStartAILogicOnPossess = false;
}

void ALB_MonsterAIController::BeginPlay()
{
	Super::BeginPlay();

	TryStartCombatLogic();
}

void ALB_MonsterAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindDeathTagEvent();
	StopCombatLogic(TEXT("Controller EndPlay"));

	Super::EndPlay(EndPlayReason);
}

void ALB_MonsterAIController::OnPossess(APawn* InPawn)
{
	// 재Possess 시 이전 Pawn ASC 구독과 예약 시작 해제
	UnbindDeathTagEvent();
	GetWorldTimerManager().ClearTimer(StartRetryHandle);

	Super::OnPossess(InPawn);

	StartRetryCount = 0;
	TryStartCombatLogic();
}

void ALB_MonsterAIController::OnUnPossess()
{
	UnbindDeathTagEvent();
	StopCombatLogic(TEXT("UnPossess"));
	TargetActor.Reset();
	// 다음 Possess에서 다시 시작할 수 있도록 초기화
	bCombatLogicStarted = false;
	StartRetryCount = 0;

	Super::OnUnPossess();
}

void ALB_MonsterAIController::TryStartCombatLogic()
{
	// 재시도 예약 중 직접 호출된 경우 중복 예약 방지
	GetWorldTimerManager().ClearTimer(StartRetryHandle);

	if (bCombatLogicStarted || !HasAuthority() || !HasActorBegunPlay())
	{
		return;
	}

	if (!StateTreeAIComponent || !CombatStateTree)
	{
		UE_LOG(LogLB, Warning, TEXT("[%s] CombatStateTree가 지정되지 않아 AI Logic을 시작하지 않음"), *GetName());
		return;
	}

	ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(GetPawn());
	if (!Monster)
	{
		return;
	}

	// Possess가 Pawn BeginPlay보다 먼저 온 경우 다음 틱에 다시 확인
	if (!IsMonsterReady(Monster))
	{
		ScheduleStartRetry();
		return;
	}

	// ASC 준비 이후 구독. 이미 사망한 경우도 같은 정리 경로 사용
	BindDeathTagEvent(Monster->GetAbilitySystemComponent());

	if (Monster->IsDead())
	{
		HandleMonsterDeath();
		return;
	}

	if (StateTreeAIComponent->IsRunning())
	{
		bCombatLogicStarted = true;
		return;
	}

	StateTreeAIComponent->SetStateTree(CombatStateTree);
	StateTreeAIComponent->StartLogic();
	bCombatLogicStarted = StateTreeAIComponent->IsRunning();

	if (!bCombatLogicStarted)
	{
		UE_LOG(LogLB, Warning, TEXT("[%s] StateTree Logic 시작 실패: %s"), *GetName(), *GetNameSafe(CombatStateTree));
	}
}

AActor* ALB_MonsterAIController::GetCombatTarget() const
{
	return TargetActor.IsValid() ? TargetActor.Get() : nullptr;
}

void ALB_MonsterAIController::SetCombatTarget(AActor* NewTarget)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!NewTarget)
	{
		TargetActor.Reset();
		return;
	}

	if (NewTarget == this || NewTarget == GetPawn())
	{
		return;
	}

	TargetActor = NewTarget;
}

bool ALB_MonsterAIController::IsMonsterReady(const ALB_MonsterBase* Monster) const
{
	if (!Monster || !Monster->HasActorBegunPlay())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	return ASC && ASC->AbilityActorInfo.IsValid() && ASC->GetAvatarActor() == Monster;
}

void ALB_MonsterAIController::ScheduleStartRetry()
{
	if (StartRetryCount >= MaxStartRetryCount)
	{
		UE_LOG(LogLB, Warning, TEXT("[%s] 몬스터 초기화 대기 %d틱 초과로 AI Logic 시작 재시도 중단"), *GetName(), MaxStartRetryCount);
		return;
	}

	++StartRetryCount;
	StartRetryHandle = GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &ThisClass::OnStartRetryTick));
}

void ALB_MonsterAIController::OnStartRetryTick()
{
	StartRetryHandle.Invalidate();
	TryStartCombatLogic();
}

void ALB_MonsterAIController::StopCombatLogic(const FString& Reason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartRetryHandle);
	}

	if (StateTreeAIComponent && StateTreeAIComponent->IsRunning())
	{
		StateTreeAIComponent->StopLogic(Reason);
	}

	StopMovement();
}

void ALB_MonsterAIController::BindDeathTagEvent(UAbilitySystemComponent* ASC)
{
	if (!HasAuthority() || !ASC)
	{
		return;
	}

	if (DeathTagBoundASC.Get() == ASC && DeathTagHandle.IsValid())
	{
		return;
	}

	UnbindDeathTagEvent();

	DeathTagHandle = ASC->RegisterGameplayTagEvent(TAG_State_Death, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnDeathTagChanged);
	DeathTagBoundASC = ASC;
}

void ALB_MonsterAIController::UnbindDeathTagEvent()
{
	if (UAbilitySystemComponent* ASC = DeathTagBoundASC.Get())
	{
		if (DeathTagHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(TAG_State_Death, EGameplayTagEventType::NewOrRemoved)
				.Remove(DeathTagHandle);
		}
	}

	DeathTagHandle.Reset();
	DeathTagBoundASC.Reset();
}

void ALB_MonsterAIController::OnDeathTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		HandleMonsterDeath();
	}
}

void ALB_MonsterAIController::HandleMonsterDeath()
{
	if (!HasAuthority())
	{
		return;
	}

	StopMovement();
	TargetActor.Reset();

	ALB_MeleeMonster* MeleeMonster = Cast<ALB_MeleeMonster>(GetPawn());
	if (!MeleeMonster)
	{
		return;
	}

	// 몬스터 근접 공격 Spec만 취소. 그 외 Ability는 건드리지 않음
	if (UAbilitySystemComponent* ASC = MeleeMonster->GetAbilitySystemComponent())
	{
		const FGameplayAbilitySpecHandle AttackHandle = MeleeMonster->FindMeleeAttackSpecHandle();
		if (AttackHandle.IsValid())
		{
			ASC->CancelAbilityHandle(AttackHandle);
		}
	}

	if (ULB_MeleeHitboxComponent* Hitbox = MeleeMonster->GetMeleeHitbox())
	{
		Hitbox->ForceEndHitWindow();
	}
}
