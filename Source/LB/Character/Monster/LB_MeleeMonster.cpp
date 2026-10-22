// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Monster/LB_MeleeMonster.h"
#include "Component/LB_MeleeHitboxComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/GA/LB_GA_MonsterMeleeAttack.h"
#include "LB.h"
#include "Net/UnrealNetwork.h"
#include "Utility/LB_NativeGameplayTag.h"
#include "Utility/LB_TeamTagUtility.h"

ALB_MeleeMonster::ALB_MeleeMonster()
{
	// Socket 기준 판정을 위해 Mesh에 부착. SocketName=None 판정은 컴포넌트가 Character 축 기준으로 처리
	MeleeHitbox = CreateDefaultSubobject<ULB_MeleeHitboxComponent>(TEXT("MeleeHitbox"));
	MeleeHitbox->SetupAttachment(GetMesh());
}

void ALB_MeleeMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALB_MeleeMonster, CombatDirectionX);
}

void ALB_MeleeMonster::BeginPlay()
{
	Super::BeginPlay();

	// OnRep가 먼저 와도 Mesh 회전은 캐시 전까지 적용하지 않으므로 여기서 읽는 값이 초기 회전. 이후 현재 복제값을 다시 적용
	if (!bBaseMeshRotationCached && GetMesh())
	{
		BaseMeshRelativeRotation = GetMesh()->GetRelativeRotation();
		bBaseMeshRotationCached = true;
	}

	ApplyCombatDirectionToHitbox();
	ApplyCombatFacingToMesh();
}

void ALB_MeleeMonster::OnRep_CombatDirectionX()
{
	ApplyCombatDirectionToHitbox();
	ApplyCombatFacingToMesh();
}

void ALB_MeleeMonster::ApplyCombatFacingToMesh()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!bBaseMeshRotationCached || !MeshComp)
	{
		return;
	}

	// 항상 캐시한 초기 회전에서 계산해 반복 전환 시 누적되지 않음
	FRotator TargetRotation = BaseMeshRelativeRotation;
	if (CombatDirectionX < 0)
	{
		TargetRotation.Yaw += 180.f;
	}

	MeshComp->SetRelativeRotation(TargetRotation);

	// 비로컬 인스턴스의 네트워크 스무딩이 ACharacter::BaseRotationOffset으로 Mesh 회전을 되돌리지 않도록 현재 방향을 기준으로 갱신
	CacheInitialMeshOffset(MeshComp->GetRelativeLocation(), TargetRotation);
}

void ALB_MeleeMonster::ApplyCombatDirectionToHitbox()
{
	if (MeleeHitbox)
	{
		MeleeHitbox->SetAttackDirectionX(static_cast<float>(CombatDirectionX));
	}
}

void ALB_MeleeMonster::SetCombatFacingFromTarget(AActor* TargetActor)
{
	if (!HasAuthority() || IsDead() || !IsValid(TargetActor) || TargetActor == this || !MeleeHitbox)
	{
		return;
	}

	const float DeltaX = TargetActor->GetActorLocation().X - GetActorLocation().X;
	if (FMath::IsNearlyZero(DeltaX))
	{
		return;
	}

	const int8 NewDirectionX = DeltaX > 0.f ? 1 : -1;
	if (NewDirectionX == CombatDirectionX)
	{
		return;
	}

	CombatDirectionX = NewDirectionX;
	ApplyCombatDirectionToHitbox();
	ApplyCombatFacingToMesh();
}

float ALB_MeleeMonster::GetCombatDirectionX() const
{
	return static_cast<float>(CombatDirectionX);
}

bool ALB_MeleeMonster::RequestMeleeAttack(AActor* TargetActor)
{
	if (!HasAuthority() || IsDead() || !IsValid(TargetActor) || TargetActor == this)
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		return false;
	}

	// 양측 Team 태그가 있고 서로 달라야 적대. Team 태그가 없는 대상은 공격하지 않음
	const FGameplayTag SourceTeam = LBTeamTagUtility::FindTeamTag(SourceASC);
	const FGameplayTag TargetTeam = LBTeamTagUtility::FindTeamTag(TargetASC);
	if (!SourceTeam.IsValid() || !TargetTeam.IsValid() || SourceTeam == TargetTeam)
	{
		return false;
	}

	if (TargetASC->HasMatchingGameplayTag(TAG_State_Death) || SourceASC->HasMatchingGameplayTag(TAG_State_Action_Attacking))
	{
		return false;
	}

	const FGameplayAbilitySpecHandle Handle = FindMeleeAttackSpecHandle();
	if (!Handle.IsValid())
	{
		return false;
	}

	// Ability는 활성화 시점의 Hitbox 방향(CombatDirectionX 적용값)을 그대로 사용하므로 활성화 전에 확정
	SetCombatFacingFromTarget(TargetActor);
	return SourceASC->TryActivateAbility(Handle, false);
}

FGameplayAbilitySpecHandle ALB_MeleeMonster::FindMeleeAttackSpecHandle() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpecHandle FoundHandle;
	int32 MatchCount = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->IsA<ULB_GA_MonsterMeleeAttack>() && Spec.Ability->GetAssetTags().HasTagExact(TAG_ATK_Normal))
		{
			FoundHandle = Spec.Handle;
			++MatchCount;
		}
	}

	if (MatchCount != 1)
	{
		UE_LOG(LogLB, Warning, TEXT("%s: ATK.Normal 몬스터 근접 공격 Spec이 %d개입니다. 정확히 1개가 부여되어야 합니다."), *GetName(), MatchCount);
		return FGameplayAbilitySpecHandle();
	}

	return FoundHandle;
}
