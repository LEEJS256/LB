// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/LB_AnimNotifyState_MeleeHitWindow.h"

#include "Components/SkeletalMeshComponent.h"

void ULB_AnimNotifyState_MeleeHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 에디터 미리보기 Actor처럼 판정 컴포넌트가 없으면 아무것도 하지 않음
	if (ULB_MeleeHitboxComponent* Hitbox = FindHitbox(MeshComp))
	{
		Hitbox->BeginHitWindow(HitboxSettings);
	}
}

void ULB_AnimNotifyState_MeleeHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (ULB_MeleeHitboxComponent* Hitbox = FindHitbox(MeshComp))
	{
		Hitbox->EndHitWindow();
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString ULB_AnimNotifyState_MeleeHitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Melee Hit Window");
}

ULB_MeleeHitboxComponent* ULB_AnimNotifyState_MeleeHitWindow::FindHitbox(const USkeletalMeshComponent* MeshComp)
{
	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<ULB_MeleeHitboxComponent>() : nullptr;
}
