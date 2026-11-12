#pragma once

#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "UI/Slate/FLB_SquareBoxStyle.h"
#include "LB_SquareBox.generated.h"

class SLB_SquareBox;
class ULB_ItemTooltip;
class ULB_DA_ItemInfo;
/** UMG용 SquareBox. 디자이너에서 자식 위젯 1개를 넣을 수 있음. */
UCLASS()
class LB_API ULB_SquareBox : public UContentWidget
{
	GENERATED_BODY()

public:
	ULB_SquareBox(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|Style")
	FLB_SquareBoxStyle Style;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|State")
	bool bSelected = false;

	UFUNCTION(BlueprintCallable, Category = "LB|SquareBox")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintCallable, Category = "LB|SquareBox")
	void SetStyle(const FLB_SquareBoxStyle& InStyle);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:

	virtual const ULB_DA_ItemInfo* GetCurrentItemInfo() const;

	UPROPERTY(EditAnywhere, Category = "Tooltip")
	TSubclassOf<ULB_ItemTooltip> ItemTooltipClass;

	UPROPERTY(Transient)
	TObjectPtr<ULB_ItemTooltip> ItemTooltipWidget;

	UFUNCTION()
	UWidget* GetItemTooltip();

	static TSubclassOf<ULB_ItemTooltip> FindDefaultTooltipClass();

	/** 자식 클래스(Button)가 다른 Slate 위젯을 만들 때 오버라이드 */
	virtual TSharedRef<SLB_SquareBox> CreateSquareBox();
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnSlotAdded(UPanelSlot* InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* InSlot) override;


	TSharedPtr<SLB_SquareBox> MySquareBox;
};
