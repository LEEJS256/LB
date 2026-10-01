// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LB_CharacterBase.h"
#include "GameFramework/Character.h"
#include "LB_PlayerCharacter.generated.h"

class ULB_GasComponent;
class USpringArmComponent;
class UCameraComponent;
class UAbilitySystemComponent;

UCLASS()
class LB_API ALB_PlayerCharacter : public ALB_CharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALB_PlayerCharacter();

	//ASC = GAS본체
	UAbilitySystemComponent* GetAbilitySystemComponent() const ;

	// ALB_PlayerController가 IA_Move의 2D Axis 값을 전달할 때 호출 (Right:+X, Up:+Y 기준)
	void AddMoveInput(const FVector2D& Axis2D);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override; // 서버
	virtual void OnRep_PlayerState() override;

	void InitGAS();
#pragma region COMPONENT_AREA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<ULB_GasComponent> GasComp;

#pragma endregion
public:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
