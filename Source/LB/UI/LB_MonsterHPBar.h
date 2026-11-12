// LB_MonsterHPBar.h
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_MonsterHPBar.generated.h"

class ULB_NeonProgressBar;

UCLASS()
class LB_API ULB_MonsterHPBar : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHealth(float Current, float Max);

protected:
	// WBP에서 이름을 HPBar로 맞춰서 NeonProgressBar 배치
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_NeonProgressBar> HPBar;
};