// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Monster/LB_MeleeMonster.h"
#include "Component/LB_MeleeHitboxComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/GA/LB_GA_MonsterMeleeAttack.h"
#include "LB.h"
#include "Utility/LB_NativeGameplayTag.h"

namespace
{
	// ASC가 보유한 Team 하위 태그(Team.Player, Team.Monster 등). 없으면 빈 태그
	FGameplayTag FindTeamTag(const UAbilitySystemComponent* ASC)
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);

		for (const FGameplayTag& Tag : OwnedTags)
		{
			if (Tag.MatchesTag(TAG_Team) && Tag != TAG_Team)
			{
				return Tag;
			}
		}
		return FGameplayTag();
	}
}

ALB_MeleeMonster::ALB_MeleeMonster()
{
	// Socket 기준 판정을 위해 Mesh에 부착. SocketName=None 판정은 컴포넌트가 Character 축 기준으로 처리
	MeleeHitbox = CreateDefaultSubobject<ULB_MeleeHitboxComponent>(TEXT("MeleeHitbox"));
	MeleeHitbox->SetupAttachment(GetMesh());
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

	MeleeHitbox->SetAttackDirectionX(DeltaX);
}

float ALB_MeleeMonster::GetCombatDirectionX() const
{
	return MeleeHitbox ? MeleeHitbox->GetAttackDirectionX() : 1.f;
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
	const FGameplayTag SourceTeam = FindTeamTag(SourceASC);
	const FGameplayTag TargetTeam = FindTeamTag(TargetASC);
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

	// Ability는 활성화 시점의 Hitbox 방향을 그대로 사용하므로 활성화 전에 확정
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
