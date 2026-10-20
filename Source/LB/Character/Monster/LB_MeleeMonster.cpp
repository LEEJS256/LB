// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Monster/LB_MeleeMonster.h"
#include "Component/LB_MeleeHitboxComponent.h"

ALB_MeleeMonster::ALB_MeleeMonster()
{
	// Socket 기준 판정을 위해 Mesh에 부착. SocketName=None 판정은 컴포넌트가 Character 축 기준으로 처리
	MeleeHitbox = CreateDefaultSubobject<ULB_MeleeHitboxComponent>(TEXT("MeleeHitbox"));
	MeleeHitbox->SetupAttachment(GetMesh());
}
