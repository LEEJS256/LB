#pragma once

#include "CoreMinimal.h"
#include "LB_ChatTypes.generated.h"

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ELB_ChatChannel : uint8
{
	None   = 0,
	All    = 1 << 0,
	Party  = 1 << 1,
	System = 1 << 2,
};
ENUM_CLASS_FLAGS(ELB_ChatChannel);

USTRUCT(BlueprintType)
struct FLB_ChatMessage
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) ELB_ChatChannel Channel = ELB_ChatChannel::All;
	UPROPERTY(BlueprintReadOnly) FString Sender;
	UPROPERTY(BlueprintReadOnly) FString Text;
	UPROPERTY(BlueprintReadOnly) float ServerTime = 0.f;
};