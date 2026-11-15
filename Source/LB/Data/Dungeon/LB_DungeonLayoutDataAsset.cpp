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

bool FLB_DungeonRouteState::ValidateRooms(const TArray<FLB_DungeonRoomDefinition>& InRooms, FString& OutError)
{
	OutError.Reset();

	if (InRooms.Num() < 1 || InRooms.Num() > MaxRooms)
	{
		OutError = FString::Printf(TEXT("Room count %d out of range [1, %d]"), InRooms.Num(), MaxRooms);
		return false;
	}

	TSet<FName> SeenIds;
	SeenIds.Reserve(InRooms.Num());

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

		bool bAlreadyInSet = false;
		SeenIds.Add(Room.RoomId, &bAlreadyInSet);
		if (bAlreadyInSet)
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

	return ValidateRooms(Rooms, OutError);
}

bool ULB_DungeonLayoutDataAsset::ValidateLayout(FString& OutError) const
{
	FString RoomError;
	if (!FLB_DungeonRouteState::ValidateRooms(Rooms, RoomError))
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

	// 원본 배열을 값 복사한 독립 Route. Layout 참조는 넣지 않는다.
	FLB_DungeonRouteState Candidate;
	Candidate.InstanceKey = InstanceKey;
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
