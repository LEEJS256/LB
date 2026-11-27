// LB_ChatComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/UI_Utility/LB_ChatTypes.h"
#include "LB_ChatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnChatReceived, const FLB_ChatMessage&, Message);

/** PlayerController에 붙는 채팅 RPC 창구. UI는 OnChatReceived만 구독한다. */
UCLASS(ClassGroup = (LB), meta = (BlueprintSpawnableComponent))
class LB_API ULB_ChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULB_ChatComponent();

	// 로컬: Panel이 호출
	UFUNCTION(BlueprintCallable, Category = "LB|Chat")
	void SendChat(ELB_ChatChannel Channel, const FString& Text);

	// 로컬 전용 시스템 메시지 (서버 안 거침: 아이템 획득, 레벨업 등)
	UFUNCTION(BlueprintCallable, Category = "LB|Chat")
	void AddLocalSystemMessage(const FString& Text);

	// 서버 전용: 전원에게 시스템 공지 (던전 클리어 등)
	static void ServerBroadcastSystem(UWorld* World, const FString& Text);

	UPROPERTY(BlueprintAssignable, Category = "LB|Chat")
	FLB_OnChatReceived OnChatReceived;

protected:
	UFUNCTION(Server, Reliable)
	void Server_SendChat(ELB_ChatChannel Channel, const FString& Text);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveChat(const FLB_ChatMessage& Message);

	UPROPERTY(EditDefaultsOnly, Category = "LB|Chat")
	int32 MaxLength = 100;

	UPROPERTY(EditDefaultsOnly, Category = "LB|Chat")
	float MinSendInterval = 0.5f;

private:
	static void Deliver(UWorld* World, const FLB_ChatMessage& Msg, const APlayerController* SenderPC);
	static bool IsSameParty(const APlayerController* A, const APlayerController* B);

	double LastSendTime = -1.0; // 서버에서만 의미 있음
};