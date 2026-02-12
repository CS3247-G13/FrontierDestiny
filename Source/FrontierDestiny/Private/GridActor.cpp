// Fill out your copyright notice in the Description page of Project Settings.


#include "GridActor.h"
#include "TowerActor.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "TowerData.h"

// Sets default values
AGridActor::AGridActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    ScaleX = 300.f;
    ScaleY = 300.f;
    CellSize = 100.f;

    // 1. Create the Root
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    

    // 2. Create the Visualizer Box
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("EditorVisualizer"));
    CollisionBox->SetupAttachment(Root);

    // Set default behavior for the visualizer
    CollisionBox->SetLineThickness(2.0f);
    CollisionBox->bHiddenInGame = true; // Only see it in the Editor
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionProfileName(TEXT("GridCollision"));
    

    
}

void AGridActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cell Size is close to 0!"))
        CellSize = 100.0f;
	}

    // Initialize
    GridSize = { FMath::FloorToInt32(ScaleX / CellSize), FMath::FloorToInt32(ScaleY / CellSize) };

	if (GridSize.X <= 0 || GridSize.Y <= 0)
    {
        GridSize.X = 1;
        GridSize.Y = 1;
        ScaleX = CellSize;
        ScaleY = CellSize;
    }

    Occupied.Empty();
    Towers.Empty();
    Occupied.Init(false, GridSize.X * GridSize.Y);
	Towers.Init(nullptr, GridSize.X * GridSize.Y);

    if (CollisionBox)
    {
        CollisionBox->SetBoxExtent(FVector(ScaleX / 2.f, ScaleY / 2.f, 10.0f));
        CollisionBox->SetRelativeLocation(FVector(ScaleX / 2.f, ScaleY / 2.f, 0.0f));
    }

}

// THIS FUNCTION ASSUMES VERTICAL GRIDS ONLY, IF WE ROTATE THE GRID, UPDATE THIS FUNCTION
bool AGridActor::GetSnappedGridIndex(const FVector& HitLocation, FIntPoint& OutGridIndex) const
{
    FVector ActorLocation = GetActorLocation();

    FIntPoint TempIndex;
    TempIndex.X = FMath::Floor((HitLocation.X - ActorLocation.X) / CellSize);
    TempIndex.Y = FMath::Floor((HitLocation.Y - ActorLocation.Y) / CellSize);
	
    // Out of bounds
    if (TempIndex.X < 0 || TempIndex.X >= GridSize.X ||
        TempIndex.Y < 0 || TempIndex.Y >= GridSize.Y)
    {
        return false;
    }
	OutGridIndex = TempIndex;
	return true;
}

void AGridActor::GetCellCenterWorldLocationFromGridIndex(const FIntPoint& GridIndex, FVector& OutLocation) const
{
    FVector ActorLocation = GetActorLocation();
    FVector DefaultOffset = FVector{ -CellSize / 2.f, -CellSize / 2.f, 0.f };

    // Account for the rotation in order to get the correct corner of the cell
    OutLocation = ActorLocation
        + FVector{ GridIndex.X * CellSize, GridIndex.Y * CellSize, 0.f }
        + -DefaultOffset;
}

bool AGridActor::GetWorldLocationFromGridIndex(const FIntPoint& GridIndex, const FRotator& Rotation, FVector& OutLocation) const
{
    FVector ActorLocation = GetActorLocation();
	FVector DefaultOffset = FVector{ -CellSize / 2.f, -CellSize / 2.f, 0.f };
	FVector RotatedOffset = Rotation.RotateVector(DefaultOffset);

	if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }
    // Account for the rotation in order to get the correct corner of the cell
    OutLocation = ActorLocation 
        + FVector{ GridIndex.X * CellSize, GridIndex.Y * CellSize, 0.f }
        + -DefaultOffset
        + RotatedOffset;
    return true;
}

FIntPoint AGridActor::RotateOffset(FIntPoint Offset, FRotator Rotation)
{
    FVector VectorOffset = FVector{ float(Offset.X), float(Offset.Y), 0.f };
    FVector RotatedVectorOffset = Rotation.RotateVector(VectorOffset);
    FIntPoint RotatedVectorOffsetInt = FIntPoint{ FMath::RoundToInt32(RotatedVectorOffset.X), FMath::RoundToInt32(RotatedVectorOffset.Y) };
    return RotatedVectorOffsetInt;
}

bool AGridActor::CanPlaceTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, UTowerData* TowerInfo)
{
    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }
    

    FIntPoint CornerGridIndex = PivotPointIndex - RotateOffset(TowerInfo->PivotPoint, Rotation);

    for (const FIntPoint& Offset : TowerInfo->Footprint)
    {
        FIntPoint TargetIndex = CornerGridIndex + RotateOffset(Offset, Rotation);

        if (TargetIndex.X < 0 || TargetIndex.X >= GridSize.X ||
            TargetIndex.Y < 0 || TargetIndex.Y >= GridSize.Y)
        {
            return false; // Out of bounds
        }

        int CalculatedIndex = TargetIndex.Y * GridSize.X + TargetIndex.X;
        if (!Occupied.IsValidIndex(CalculatedIndex) || Occupied[CalculatedIndex])
        {
            return false; // Already filled
        }
    }
    return true;
}

bool AGridActor::PlaceTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, ATowerActor* TowerPtr)
{
    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }
	// if (!CanPlaceTower(CornerGridIndex, Rotation, TowerPtr->TowerInfo))
    // {
        // Allow overwrite, should check CanPlaceTower first if you want to avoid this
        // return false;
    // }
	if (!TowerPtr || !TowerPtr->TowerInfo)
    {
        return false;
    }

    FIntPoint CornerGridIndex = PivotPointIndex - RotateOffset(TowerPtr->TowerInfo->PivotPoint, Rotation);

    for (const FIntPoint& Offset : TowerPtr->TowerInfo->Footprint)
    {
        FIntPoint TargetIndex = CornerGridIndex + RotateOffset(Offset, Rotation);

        int CalculatedIndex = TargetIndex.Y * GridSize.X + TargetIndex.X;

        Occupied[CalculatedIndex] = true;
        Towers[CalculatedIndex] = TowerPtr;
    }
	return true;
}

bool AGridActor::RemoveTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, ATowerActor* TowerPtr)
{
    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }
    FIntPoint CornerGridIndex = PivotPointIndex - RotateOffset(TowerPtr->TowerInfo->PivotPoint, Rotation);
    for (const FIntPoint& Offset : TowerPtr->TowerInfo->Footprint)
    {
        FIntPoint TargetIndex = CornerGridIndex + RotateOffset(Offset, Rotation);

        int CalculatedIndex = TargetIndex.Y * GridSize.X + TargetIndex.X;

        Occupied[CalculatedIndex] = false;
        Towers[CalculatedIndex] = nullptr;
    }
    return true;
}



