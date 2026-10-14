// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_PlayerController.h"
#include "PlayerState/LB_PlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "Character/LB_PlayerCharacter.h"
#include "UI/LB_PlayerStatus.h"
#include "UI/LB_HUD.h"


void ALB_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>())
		{
			if (MoveMappingContext)
			{
				Subsystem->AddMappingContext(MoveMappingContext, 0);
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,
			                                   &ALB_PlayerController::OnMoveTriggered);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this,
			                                   &ALB_PlayerController::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this,
			                                   &ALB_PlayerController::StopJumping);
		}
		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this,
			                                   &ALB_PlayerController::StartSprint);
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Completed, this,
			                                   &ALB_PlayerController::StopSprint);
		}
		if (NormalAttackAction)
		{
			EnhancedInputComponent->BindAction(NormalAttackAction, ETriggerEvent::Started, this,
			                                   &ALB_PlayerController::OnNormalAttackStarted);
		}
		if (StrongAttackAction)
		{
			EnhancedInputComponent->BindAction(StrongAttackAction, ETriggerEvent::Completed, this,
			                                   &ALB_PlayerController::StrongATK);
		}
	}
}

void ALB_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitUI();
	BindHUD();
}

void ALB_PlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitUI();
	BindHUD();
}

void ALB_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	InitUI();
	BindHUD();
}

void ALB_PlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	if (ALB_PlayerCharacter* LBCharacter = Cast<ALB_PlayerCharacter>(GetPawn()))
	{
		LBCharacter->AddMoveInput(Value.Get<FVector2D>());
	}
}

void ALB_PlayerController::InitUI()
{
	if (HUDWidget || !IsLocalController() || !HUDWidgetClass) return;

	HUDWidget = CreateWidget<ULB_HUD>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void ALB_PlayerController::BindHUD()
{
	if (!HUDWidget)
		return;

	ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>();
	if (!PS)
		return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
		return;



	if (ULB_PlayerStatus* Status = HUDWidget->GetLocalPlayerStatus())
	{
		Status->BindToASC(ASC);  
	}
	

	
}

void ALB_PlayerController::Jump(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
		return;

	ALB_PlayerCharacter* PlayerCharacter = Cast<ALB_PlayerCharacter>(ControlledPawn);
	if (!IsValid(PlayerCharacter))
		return;


	PlayerCharacter->Jump();
}

void ALB_PlayerController::StopJumping()
{
	if (ACharacter* pCharacter = Cast<ACharacter>(GetPawn()))
	{
		pCharacter->StopJumping();
	}
}

void ALB_PlayerController::StartSprint(const FInputActionValue& Value)
{
}

void ALB_PlayerController::StopSprint(const FInputActionValue& Value)
{
}

void ALB_PlayerController::OnNormalAttackStarted(const FInputActionValue& Value)
{
	// 공격 입력 1회당 기본 공격 활성화를 1회 요청 (실행 가능 여부는 GAS/서버가 확정)
	if (ALB_PlayerCharacter* LBCharacter = Cast<ALB_PlayerCharacter>(GetPawn()))
	{
		LBCharacter->RequestNormalAttack();
	}
}

void ALB_PlayerController::StrongATK(const FInputActionValue& Value)
{
	ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>();
	if (!PS)
		return;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		PS,
		TAG_Input_RightClick,
		FGameplayEventData()
	);

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
		return;

	FGameplayTagContainer GATagContainer;
	GATagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("ATK.Strong")));

	ASC->TryActivateAbilitiesByTag(GATagContainer);
}
