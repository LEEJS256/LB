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

	// StartRoom에서 도달 가능한 방을 표시한다(방향 그래프, 순환 허용).
	// 이미 방문한 방은 다시 탐색하지 않으므로 순환이 있어도 종료한다.
	// 호출 전에 모든 TargetRoomId 존재가 검증되어 있어야 한다.
	static void MarkReachableRooms(int32 StartIndex,
		const TArray<FLB_DungeonRoomDefinition>& InRooms,
		const TMap<FName, int32>& IndexById,
		TArray<bool>& OutVisited)
	{
		OutVisited.Init(false, InRooms.Num());

		TArray<int32> Stack;
		Stack.Add(StartIndex);
		OutVisited[StartIndex] = true;

		while (Stack.Num() > 0)
		{
			const int32 RoomIndex = Stack.Pop(EAllowShrinking::No);
			for (const FLB_DungeonExitLink& Exit : InRooms[RoomIndex].Exits)
			{
				const int32 TargetIndex = IndexById.FindChecked(Exit.TargetRoomId);
				if (!OutVisited[TargetIndex])
				{
					OutVisited[TargetIndex] = true;
					Stack.Add(TargetIndex);
				}
			}
		}
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

bool FLB_DungeonRouteState::RoomHasEntry(const FLB_DungeonRoomDefinition& Room, FName EntryId)
{
	return !EntryId.IsNone() && Room.EntryIds.Contains(EntryId);
}

bool FLB_DungeonRouteState::ValidateGraph(FName InStartRoomId, FName InStartEntryId, const TArray<FLB_DungeonRoomDefinition>& InRooms, FString& OutError)
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

		// EntryId는 방 내부에서만 고유하다(다른 방에서 같은 이름 재사용 허용).
		if (Room.EntryIds.Num() < 1 || Room.EntryIds.Num() > MaxEntriesPerRoom)
		{
			OutError = FString::Printf(TEXT("Room '%s': entry count %d out of range [1, %d]"),
				*Room.RoomId.ToString(), Room.EntryIds.Num(), MaxEntriesPerRoom);
			return false;
		}
		TSet<FName> SeenEntryIds;
		SeenEntryIds.Reserve(Room.EntryIds.Num());
		for (int32 EntryIndex = 0; EntryIndex < Room.EntryIds.Num(); ++EntryIndex)
		{
			const FName EntryId = Room.EntryIds[EntryIndex];
			if (!IsValidIdentifier(EntryId))
			{
				OutError = FString::Printf(TEXT("Room '%s' EntryIds[%d]: '%s' must be non-None and contain only letters, digits, underscore"),
					*Room.RoomId.ToString(), EntryIndex, *EntryId.ToString());
				return false;
			}
			bool bEntryAlreadyInSet = false;
			SeenEntryIds.Add(EntryId, &bEntryAlreadyInSet);
			if (bEntryAlreadyInSet)
			{
				OutError = FString::Printf(TEXT("Room '%s' EntryIds[%d]: duplicate EntryId '%s'"),
					*Room.RoomId.ToString(), EntryIndex, *EntryId.ToString());
				return false;
			}
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

	// StartEntryId 형식/StartRoom 선언 검사.
	if (!IsValidIdentifier(InStartEntryId))
	{
		OutError = FString::Printf(TEXT("StartEntryId '%s' must be non-None and contain only letters, digits, underscore"),
			*InStartEntryId.ToString());
		return false;
	}
	if (!RoomHasEntry(InRooms[*StartIndex], InStartEntryId))
	{
		OutError = FString::Printf(TEXT("StartEntryId '%s' is not declared in StartRoom '%s' EntryIds"),
			*InStartEntryId.ToString(), *InStartRoomId.ToString());
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

			const int32* TargetIndex = IndexById.Find(Exit.TargetRoomId);
			if (!TargetIndex)
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit '%s': TargetRoomId '%s' does not exist"),
					*RoomName, *Exit.ExitId.ToString(), *Exit.TargetRoomId.ToString());
				return false;
			}

			if (!IsValidIdentifier(Exit.TargetEntryId))
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit '%s': TargetEntryId '%s' must be non-None and contain only letters, digits, underscore"),
					*RoomName, *Exit.ExitId.ToString(), *Exit.TargetEntryId.ToString());
				return false;
			}

			if (!RoomHasEntry(InRooms[*TargetIndex], Exit.TargetEntryId))
			{
				OutError = FString::Printf(TEXT("Room '%s' Exit '%s': TargetEntryId '%s' is not declared in target room '%s' EntryIds"),
					*RoomName, *Exit.ExitId.ToString(), *Exit.TargetEntryId.ToString(), *Exit.TargetRoomId.ToString());
				return false;
			}
		}
	}

	// StartRoom에서 도달 가능성 확인. 다른 방을 경유한 순환(A → B → A)은 허용한다.
	TArray<bool> Visited;
	LB_DungeonRoute::MarkReachableRooms(*StartIndex, InRooms, IndexById, Visited);

	for (int32 Index = 0; Index < InRooms.Num(); ++Index)
	{
		if (!Visited[Index])
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

	return ValidateGraph(StartRoomId, StartEntryId, Rooms, OutError);
}

bool ULB_DungeonLayoutDataAsset::ValidateLayout(FString& OutError) const
{
	FString RoomError;
	if (!FLB_DungeonRouteState::ValidateGraph(StartRoomId, StartEntryId, Rooms, RoomError))
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
	Candidate.StartEntryId = StartEntryId;
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
