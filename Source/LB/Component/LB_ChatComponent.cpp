// LB_ChatComponent.cpp
#include "Component/LB_ChatComponent.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

ULB_ChatComponent::ULB_ChatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // RPC 필수
}

void ULB_ChatComponent::SendChat(ELB_ChatChannel Channel, const FString& Text)
{
	if (Channel != ELB_ChatChannel::All && Channel != ELB_ChatChannel::Party)
		return;

	const FString Trimmed = Text.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
		return;

	Server_SendChat(Channel, Trimmed.Left(MaxLength));
}

void ULB_ChatComponent::Server_SendChat_Implementation(ELB_ChatChannel Channel, const FString& Text)
{
	APlayerController* PC = GetOwner<APlayerController>();
	if (!PC || !PC->PlayerState)
		return;

	// 클라 값은 믿지 않고 서버에서 다시 검증
	if (Channel != ELB_ChatChannel::All && Channel != ELB_ChatChannel::Party)
		return;

	const double Now = GetWorld()->GetTimeSeconds();
	if (LastSendTime >= 0.0 && Now - LastSendTime < MinSendInterval)
		return;
	LastSendTime = Now;

	FLB_ChatMessage Msg;
	Msg.Channel = Channel;
	Msg.Sender = PC->PlayerState->GetPlayerName(); // 닉네임 위조 방지
	Msg.Text = Text.TrimStartAndEnd().Left(MaxLength);
	if (Msg.Text.IsEmpty())
		return;

	if (const AGameStateBase* GS = GetWorld()->GetGameState())
		Msg.ServerTime = GS->GetServerWorldTimeSeconds();

	Deliver(GetWorld(), Msg, PC);
}

void ULB_ChatComponent::Client_ReceiveChat_Implementation(const FLB_ChatMessage& Message)
{
	OnChatReceived.Broadcast(Message);
}

void ULB_ChatComponent::AddLocalSystemMessage(const FString& Text)
{
	FLB_ChatMessage Msg;
	Msg.Channel = ELB_ChatChannel::System;
	Msg.Text = Text;
	OnChatReceived.Broadcast(Msg);
}

void ULB_ChatComponent::ServerBroadcastSystem(UWorld* World, const FString& Text)
{
	if (!World || World->GetNetMode() == NM_Client)
		return;

	FLB_ChatMessage Msg;
	Msg.Channel = ELB_ChatChannel::System;
	Msg.Text = Text;
	Deliver(World, Msg, nullptr);
}

void ULB_ChatComponent::Deliver(UWorld* World, const FLB_ChatMessage& Msg, const APlayerController* SenderPC)
{
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Target = It->Get();
		if (!Target)
			continue;

		if (Msg.Channel == ELB_ChatChannel::Party && !IsSameParty(SenderPC, Target))
			continue;

		if (ULB_ChatComponent* Chat = Target->FindComponentByClass<ULB_ChatComponent>())
			Chat->Client_ReceiveChat(Msg);
	}
}

bool ULB_ChatComponent::IsSameParty(const APlayerController* A, const APlayerController* B)
{
	// TODO: 파티 시스템 생기면 PlayerState의 PartyId 비교로 교체
	return true;
}