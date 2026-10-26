// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/LB_AN_SetMovementMode.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULB_AN_SetMovementMode::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                    const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	if (ACharacter* Char = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->SetMovementMode(NewMovementMode, CustomMode);
		}
	}
}


