// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/LB_ANS_NextSection.h"

#include "Character/LB_PlayerCharacter.h"
#include "Component/LB_BattleComponent.h"


static ULB_BattleComponent* GetBattleComp(USkeletalMeshComponent* MeshComp)
{
	ALB_PlayerCharacter* Character = MeshComp ? Cast<ALB_PlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	return Character ? Character->GetBattleComponent() : nullptr;
}

void ULB_ANS_NextSection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                      float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (ULB_BattleComponent* BattleComp = GetBattleComp(MeshComp))
	{
		BattleComp->OpenComboWindow();
	}
}

void ULB_ANS_NextSection::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{

	Super::NotifyEnd(MeshComp, Animation, EventReference);

	ULB_BattleComponent* BattleComp = GetBattleComp(MeshComp);
	if (!BattleComp) return;

	FName NextSection;
	if (!BattleComp->CloseComboWindow(NextSection)) return; // 입력 없으면 그냥 몽타주 끝남

	UAnimInstance* Anim = MeshComp->GetAnimInstance();
	UAnimMontage* Montage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
	if (!Montage)
		return;

	const FName Current = Anim->Montage_GetCurrentSection(Montage);
	//섹션 다끝나고 넘어감
	// Anim->Montage_SetNextSection(Current, NextSection, Montage);
	//NotifyEnd일때 넘어감
	Anim->Montage_JumpToSection(NextSection, Montage);
}


