// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LB_CharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ALB_CharacterBase::ALB_CharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // 필요한 서브클래스에서만 켜기
	bReplicates = true;

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

}

// Called when the game starts or when spawned
void ALB_CharacterBase::BeginPlay()
{
	Super::BeginPlay();

}

