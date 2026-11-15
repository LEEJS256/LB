// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/Dungeon/LB_DungeonLayoutDataAsset.h"

namespace LB_DungeonRoute
{
	static bool IsIdentifierChar(TCHAR C)
	{
		return (C >= TEXT('a') && C <= TEXT('z'))
			|| (C >= TEXT('A') && C <= TEXT('Z'))
			|| (C >= TEXT('0') && C <= TEXT('9'))
			|| C == TEXT('_');
	}

	enum class EVisitState : uint8
	{
		Unvisited,
		InStack,
		Done
	};

	// StartRoom에서 DFS. 방문 상태를 채우고 InStack 노드로 되돌아가는 간선(Cycle)을 발견하면 false.
	// 방 수가 MaxRooms(32) 이하이므로 재귀 깊이는 제한된다.
	static bool VisitAcyclic(int32 RoomIndex,
		const TArray<FLB_DungeonRoomDefinition>& InRooms,
		const TMap<FName, int32>& IndexById,
		TArray<EVisitState>& States,
		FString& OutError)
	{
		States[RoomIndex] = EVisitState::InStack;

		const FLB_DungeonRoomDefinition& Room = InRooms[RoomIndex];
		for (const FLB_DungeonExitLink& Exit : Room.Exits)
		{
			const int32 TargetIndex = IndexById.FindChecked(Exit.TargetRoomId);
			if (States[TargetIndex] == EVisitState::InStack)
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit '%s': cycle detected via '%s'"),
					*Room.RoomId.ToString(), *Exit.ExitId.ToString(), *Exit.TargetRoomId.ToString());
				return false;
			}
			if (States[TargetIndex] == EVisitState::Unvisited)
			{
				if (!VisitAcyclic(TargetIndex, InRooms, IndexById, States, OutError))
				{
					return false;
				}
			}
			// Done: 분기 후 합류(DAG)이므로 허용.
		}

		States[RoomIndex] = EVisitState::Done;
		return true;
	}
}

bool FLB_DungeonRouteState::IsValidIdentifier(const FName& InName)
{
	if (InName.IsNone())
	{
		return false;
	}

	const FString NameString = InName.ToString();
	if (NameString.IsEmpty())
	{
		return false;
	}

	for (const TCHAR C : NameString)
	{
		if (!LB_DungeonRoute::IsIdentifierChar(C))
		{
			return false;
		}
	}
	return true;
}

bool FLB_DungeonRouteState::ValidateGraph(FName InStartRoomId, const TArray<FLB_DungeonRoomDefinition>& InRooms, FString& OutError)
{
	OutError.Reset();

	if (InRooms.Num() < 1 || InRooms.Num() > MaxRooms)
	{
		OutError = FString::Printf(TEXT("Room count %d out of range [1, %d]"), InRooms.Num(), MaxRooms);
		return false;
	}

	TMap<FName, int32> IndexById;
	IndexById.Reserve(InRooms.Num());

	for (int32 Index = 0; Index < InRooms.Num(); ++Index)
	{
		const FLB_DungeonRoomDefinition& Room = InRooms[Index];

		if (Room.RoomId.IsNone())
		{
			OutError = FString::Printf(TEXT("Room[%d]: RoomId is None"), Index);
			return false;
		}

		if (!IsValidIdentifier(Room.RoomId))
		{
			OutError = FString::Printf(TEXT("Room[%d]: RoomId '%s' must contain only letters, digits, underscore"),
				Index, *Room.RoomId.ToString());
			return false;
		}

		if (IndexById.Contains(Room.RoomId))
		{
			OutError = FString::Printf(TEXT("Room[%d]: duplicate RoomId '%s'"), Index, *Room.RoomId.ToString());
			return false;
		}

		// 같은 맵을 다른 RoomId로 반복 사용하는 것은 허용하므로 Level 중복은 검사하지 않는다.
		if (Room.Level.IsNull())
		{
			OutError = FString::Printf(TEXT("Room[%d] '%s': Level is null"), Index, *Room.RoomId.ToString());
			return false;
		}

		IndexById.Add(Room.RoomId, Index);
	}

	// StartRoomId 형식/존재 검사.
	if (!IsValidIdentifier(InStartRoomId))
	{
		OutError = FString::Printf(TEXT("StartRoomId '%s' must be non-None and contain only letters, digits, underscore"),
			*InStartRoomId.ToString());
		return false;
	}
	const int32* StartIndex = IndexById.Find(InStartRoomId);
	if (StartIndex == nullptr)
	{
		OutError = FString::Printf(TEXT("StartRoomId '%s' does not exist in Rooms"), *InStartRoomId.ToString());
		return false;
	}

	// 방별 Exit 개수/ID/목적지 검사. 모든 목적지가 존재함을 보장한 뒤 그래프 탐색을 한다.
	for (int32 Index = 0; Index < InRooms.Num(); ++Index)
	{
		const FLB_DungeonRoomDefinition& Room = InRooms[Index];
		const FString RoomName = Room.RoomId.ToString();

		if (Room.Exits.Num() > MaxExitsPerRoom)
		{
			OutError = FString::Printf(TEXT("Room '%s': exit count %d out of range [0, %d]"),
				*RoomName, Room.Exits.Num(), MaxExitsPerRoom);
			return false;
		}

		TSet<FName> SeenExitIds;
		SeenExitIds.Reserve(Room.Exits.Num());

		for (int32 ExitIndex = 0; ExitIndex < Room.Exits.Num(); ++ExitIndex)
		{
			const FLB_DungeonExitLink& Exit = Room.Exits[ExitIndex];

			if (!IsValidIdentifier(Exit.ExitId))
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit[%d]: ExitId '%s' must be non-None and contain only letters, digits, underscore"),
					*RoomName, ExitIndex, *Exit.ExitId.ToString());
				return false;
			}

			bool bExitAlreadyInSet = false;
			SeenExitIds.Add(Exit.ExitId, &bExitAlreadyInSet);
			if (bExitAlreadyInSet)
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit[%d]: duplicate ExitId '%s'"),
					*RoomName, ExitIndex, *Exit.ExitId.ToString());
				return false;
			}

			if (!IsValidIdentifier(Exit.TargetRoomId))
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit '%s': TargetRoomId '%s' must be non-None and contain only letters, digits, underscore"),
					*RoomName, *Exit.ExitId.ToString(), *Exit.TargetRoomId.ToString());
				return false;
			}

			if (Exit.TargetRoomId == Room.RoomId)
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit '%s': self link is not allowed"),
					*RoomName, *Exit.ExitId.ToString());
				return false;
			}

			if (!IndexById.Contains(Exit.TargetRoomId))
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit '%s': TargetRoomId '%s' does not exist"),
					*RoomName, *Exit.ExitId.ToString(), *Exit.TargetRoomId.ToString());
				return false;
			}
		}
	}

	// StartRoom에서 DFS: Cycle 거부 + 도달 가능성 확인.
	TArray<LB_DungeonRoute::EVisitState> States;
	States.Init(LB_DungeonRoute::EVisitState::Unvisited, InRooms.Num());

	if (!LB_DungeonRoute::VisitAcyclic(*StartIndex, InRooms, IndexById, States, OutError))
	{
		return false;
	}

	for (int32 Index = 0; Index < InRooms.Num(); ++Index)
	{
		if (States[Index] != LB_DungeonRoute::EVisitState::Done)
		{
			OutError = FString::Printf(TEXT("Room '%s' is unreachable from StartRoomId '%s'"),
				*InRooms[Index].RoomId.ToString(), *InStartRoomId.ToString());
			return false;
		}
	}

	return true;
}

bool FLB_DungeonRouteState::Validate(FString& OutError) const
{
	OutError.Reset();

	if (!IsValidIdentifier(InstanceKey))
	{
		OutError = FString::Printf(TEXT("InstanceKey '%s' must be non-None and contain only letters, digits, underscore"),
			*InstanceKey.ToString());
		return false;
	}

	return ValidateGraph(StartRoomId, Rooms, OutError);
}

bool ULB_DungeonLayoutDataAsset::ValidateLayout(FString& OutError) const
{
	FString RoomError;
	if (!FLB_DungeonRouteState::ValidateGraph(StartRoomId, Rooms, RoomError))
	{
		OutError = FString::Printf(TEXT("Layout '%s': %s"), *GetName(), *RoomError);
		return false;
	}

	OutError.Reset();
	return true;
}

bool ULB_DungeonLayoutDataAsset::BuildRoute(FName InstanceKey, FLB_DungeonRouteState& OutRoute, FString& OutError) const
{
	OutRoute = FLB_DungeonRouteState();
	OutError.Reset();

	if (!ValidateLayout(OutError))
	{
		return false;
	}

	// 원본 Graph(방 + Exits 배열)를 값 복사한 독립 Route. Layout 참조는 넣지 않는다.
	FLB_DungeonRouteState Candidate;
	Candidate.InstanceKey = InstanceKey;
	Candidate.StartRoomId = StartRoomId;
	Candidate.Rooms = Rooms;

	FString RouteError;
	if (!Candidate.Validate(RouteError))
	{
		OutError = FString::Printf(TEXT("Layout '%s': %s"), *GetName(), *RouteError);
		return false;
	}

	OutRoute = MoveTemp(Candidate);
	return true;
}
