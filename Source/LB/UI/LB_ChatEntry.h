// LB_ChatEntry.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI/UI_Utility/LB_ChatTypes.h"
#include "LB_ChatEntry.generated.h"

class UTextBlock;

/** ListView 아이템 데이터 */
UCLASS()
class LB_API ULB_ChatItemObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "LB|Chat")
	FLB_ChatMessage Message;

	UPROPERTY(BlueprintReadOnly, Category = "LB|Chat")   // ← 여기로 이동
	FDateTime LocalTime;
};

/** 채팅 한 줄 */
UCLASS(Abstract)
class LB_API ULB_ChatEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Color")
	FSlateColor AllColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Color")
	FSlateColor PartyColor = FLinearColor(0.3f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Color")
	FSlateColor SystemColor = FLinearColor(1.f, 0.85f, 0.3f);
	

	UPROPERTY(EditAnywhere, Category = "LB|Chat")
	bool bShowTimestamp = true;
};