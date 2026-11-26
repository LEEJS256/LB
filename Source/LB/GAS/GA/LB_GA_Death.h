// LB_GA_Death.h
#pragma once
#include "CoreMinimal.h"
#include "LB_GameplayAbility.h"
#include "LB_GA_Death.generated.h"

UCLASS()
class LB_API ULB_GA_Death : public ULB_GameplayAbility
{
	GENERATED_BODY()
public:
	ULB_GA_Death();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="LB|Death")
	TObjectPtr<UAnimMontage> DeathMontage;


private:
	UFUNCTION() void OnMontageEnded();
	void FinishDeath();

	TWeakObjectPtr<AActor> Killer;
};