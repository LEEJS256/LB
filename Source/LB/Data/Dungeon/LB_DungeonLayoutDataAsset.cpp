// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/Dungeon/LB_DungeonLayoutDataAsset.h"

namespace LB_DungeonAssembly
{
	static bool IsIdentifierChar(TCHAR C)
	{
		return (C >= TEXT('a') && C <= TEXT('z'))
			|| (C >= TEXT('A') && C <= TEXT('Z'))
			|| (C >= TEXT('0') && C <= TEXT('9'))
			|| C == TEXT('_');
	}

	static bool ValidateTransform(const FTransform& InTransform, FString& OutError)
	{
		const FVector Location = InTransform.GetLocation();
		if (!FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y) || !FMath::IsFinite(Location.Z))
		{
			OutError = TEXT("location is not finite");
			return false;
		}

		// NaN이 섞이면 Equals가 false이므로 비유한 회전/스케일도 함께 거부된다.
		if (!InTransform.GetRotation().Equals(FQuat::Identity, KINDA_SMALL_NUMBER))
		{
			OutError = TEXT("rotation must be zero");
			return false;
		}

		if (!InTransform.GetScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER))
		{
			OutError = TEXT("scale must be 1");
			return false;
		}

		return true;
	}
}

bool FLB_DungeonAssemblyState::IsValidIdentifier(const FName& InName)
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
		if (!LB_DungeonAssembly::IsIdentifierChar(C))
		{
			return false;
		}
	}
	return true;
}

bool FLB_DungeonAssemblyState::ValidatePieces(const TArray<FLB_DungeonPiecePlacement>& InPieces, FString& OutError)
{
	OutError.Reset();

	if (InPieces.Num() < 1 || InPieces.Num() > MaxPieces)
	{
		OutError = FString::Printf(TEXT("Piece count %d out of range [1, %d]"), InPieces.Num(), MaxPieces);
		return false;
	}

	TSet<FName> SeenIds;
	SeenIds.Reserve(InPieces.Num());

	for (int32 Index = 0; Index < InPieces.Num(); ++Index)
	{
		const FLB_DungeonPiecePlacement& Piece = InPieces[Index];

		if (Piece.PieceId.IsNone())
		{
			OutError = FString::Printf(TEXT("Piece[%d]: PieceId is None"), Index);
			return false;
		}

		if (!IsValidIdentifier(Piece.PieceId))
		{
			OutError = FString::Printf(TEXT("Piece[%d]: PieceId '%s' must contain only letters, digits, underscore"),
				Index, *Piece.PieceId.ToString());
			return false;
		}

		bool bAlreadyInSet = false;
		SeenIds.Add(Piece.PieceId, &bAlreadyInSet);
		if (bAlreadyInSet)
		{
			OutError = FString::Printf(TEXT("Piece[%d]: duplicate PieceId '%s'"), Index, *Piece.PieceId.ToString());
			return false;
		}

		if (Piece.Level.IsNull())
		{
			OutError = FString::Printf(TEXT("Piece[%d] '%s': Level is null"), Index, *Piece.PieceId.ToString());
			return false;
		}

		FString TransformError;
		if (!LB_DungeonAssembly::ValidateTransform(Piece.Transform, TransformError))
		{
			OutError = FString::Printf(TEXT("Piece[%d] '%s': %s"), Index, *Piece.PieceId.ToString(), *TransformError);
			return false;
		}
	}

	return true;
}

bool FLB_DungeonAssemblyState::Validate(FString& OutError) const
{
	OutError.Reset();

	if (!IsValidIdentifier(InstanceKey))
	{
		OutError = FString::Printf(TEXT("InstanceKey '%s' must be non-None and contain only letters, digits, underscore"),
			*InstanceKey.ToString());
		return false;
	}

	return ValidatePieces(Pieces, OutError);
}

bool ULB_DungeonLayoutDataAsset::ValidateLayout(FString& OutError) const
{
	FString PieceError;
	if (!FLB_DungeonAssemblyState::ValidatePieces(Pieces, PieceError))
	{
		OutError = FString::Printf(TEXT("Layout '%s': %s"), *GetName(), *PieceError);
		return false;
	}

	OutError.Reset();
	return true;
}

bool ULB_DungeonLayoutDataAsset::BuildAssembly(FName InstanceKey, FLB_DungeonAssemblyState& OutState, FString& OutError) const
{
	OutState = FLB_DungeonAssemblyState();
	OutError.Reset();

	if (!ValidateLayout(OutError))
	{
		return false;
	}

	// 원본 배열을 복사한 독립 State. Layout 참조는 넣지 않는다.
	FLB_DungeonAssemblyState Candidate;
	Candidate.InstanceKey = InstanceKey;
	Candidate.Pieces = Pieces;

	FString StateError;
	if (!Candidate.Validate(StateError))
	{
		OutError = FString::Printf(TEXT("Layout '%s': %s"), *GetName(), *StateError);
		return false;
	}

	OutState = MoveTemp(Candidate);
	return true;
}
