// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_PlayerCharacter.h"

#include "Component/LB_GasComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PlayerState/LB_PlayerState.h"

// Sets default values
ALB_PlayerCharacter::ALB_PlayerCharacter()
{
	// 이동/입력은 Enhanced Input 이벤트로 처리하므로 Tick 불필요 (ALB_CharacterBase 기본값인 false 유지)

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	GasComp = CreateDefaultSubobject<ULB_GasComponent>(TEXT("GasComp"));
	
}

UAbilitySystemComponent* ALB_PlayerCharacter::GetAbilitySystemComponent() const
{
	return GasComp ? GasComp->GetASC() : nullptr;
}

// Called when the game starts or when spawned
void ALB_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ALB_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitGAS();
}

void ALB_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitGAS();
}

void ALB_PlayerCharacter::InitGAS()
{
	if (ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>())
	{
		GasComp->InitFromPlayerState(PS);
	}
}

void ALB_PlayerCharacter::AddMoveInput(const FVector2D& Axis2D)
{
	// 방향키 대각선 입력이 축 단독 입력보다 빨라지지 않도록 최대 크기 1로 clamp
	const FVector2D ClampedAxis = Axis2D.GetClampedToMaxSize(1.f);

	if (ClampedAxis.IsNearlyZero())
	{
		return;
	}

	// 월드 X: 화면 좌우(Right:+X / Left:-X), 월드 Y: 화면 깊이(Up:+Y / Down:-Y)
	AddMovementInput(FVector::ForwardVector, ClampedAxis.X);
	AddMovementInput(FVector::RightVector, ClampedAxis.Y);
}

// Called to bind functionality to input
void ALB_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

