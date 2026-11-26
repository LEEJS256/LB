// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LB_CharacterBase.generated.h"

class UAbilitySystemComponent;
struct FGameplayTag;
UCLASS()
class LB_API ALB_CharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALB_CharacterBase();
	virtual void OnDeathFinished(AActor* Killer);   // 서버
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void BindDeathTag(UAbilitySystemComponent* ASC); // ASC 초기화 직후 호출 (서버/클라 둘 다)
	virtual void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
