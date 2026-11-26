// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LB_CharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utility/LB_NativeGameplayTag.h"

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

void ALB_CharacterBase::OnDeathFinished(AActor* Killer)
{
}

// Called when the game starts or when spawned
void ALB_CharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALB_CharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ALB_CharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ALB_CharacterBase::BindDeathTag(UAbilitySystemComponent* ASC)
{
	ASC->RegisterGameplayTagEvent(TAG_State_Death, EGameplayTagEventType::NewOrRemoved)
	   .AddUObject(this, &ThisClass::OnDeadTagChanged);
}

void ALB_CharacterBase::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bDead = NewCount > 0;

	GetCharacterMovement()->StopMovementImmediately();
	bDead ? GetCharacterMovement()->DisableMovement()
		  : GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, bDead ? ECR_Ignore : ECR_Block);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		bDead ? DisableInput(PC) : EnableInput(PC);
}
