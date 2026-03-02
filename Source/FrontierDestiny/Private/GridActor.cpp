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
    BoundaryOccupied.Empty();
    Occupied.Init(false, GridSize.X * GridSize.Y);
	Towers.Init(nullptr, GridSize.X * GridSize.Y);
	BoundaryOccupied.Init(0, GridSize.X * GridSize.Y);

    if (CollisionBox)
    {
        CollisionBox->SetBoxExtent(FVector(ScaleX / 2.f, ScaleY / 2.f, 10.0f));
        CollisionBox->SetRelativeLocation(FVector(ScaleX / 2.f, ScaleY / 2.f, 0.0f));
    }

}

// THIS FUNCTION ASSUMES VERTICAL GRIDS ONLY, IF WE ROTATE THE GRID, UPDATE THIS FUNCTION
bool AGridActor::GetSnappedGridIndex(const FVector& HitLocation, FIntPoint& OutGridIndex) const
{
    FVector TransformedHitLocation = GetActorTransform().InverseTransformPosition(HitLocation);

    FIntPoint TempIndex;
    TempIndex.X = FMath::Floor((TransformedHitLocation.X) / CellSize);
    TempIndex.Y = FMath::Floor((TransformedHitLocation.Y) / CellSize);
	
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
    FVector LocalOffset = FVector{ GridIndex.X * CellSize + CellSize / 2.f, GridIndex.Y * CellSize + CellSize / 2.f, 0.f };
    // Account for the rotation in order to get the correct corner of the cell
    OutLocation = GetActorTransform().TransformPosition(LocalOffset);
}

bool AGridActor::GetWorldLocationFromGridIndex(const FIntPoint& GridIndex, const FRotator& Rotation, FVector& OutLocation) const
{
	FVector DefaultOffset = FVector{ -CellSize / 2.f, -CellSize / 2.f, 0.f };
	FVector RotatedOffset = Rotation.RotateVector(DefaultOffset);

	if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }
    // Account for the rotation in order to get the correct corner of the cell
    OutLocation = GetActorTransform().TransformPosition(
        FVector{ GridIndex.X * CellSize, GridIndex.Y * CellSize, 0.f }
        + -DefaultOffset
        + RotatedOffset);

    return true;
}

bool AGridActor::GetTowerPlacementLocationFromGridIndex(const FIntPoint& PivotPointIndex, const FRotator& Rotation, const FTowerData& TowerData, FTransform& OutTransform) const
{
    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }

    TSet<FVector> LocationsToCheck;
    FVector AddedLocation;
    
    TArray<int32> FootprintIndices;
    TArray<int32> BoundaryIndices;
    GetTowerGridIndices(PivotPointIndex, Rotation, TowerData, FootprintIndices, BoundaryIndices);

    for (const int32& Footprint : FootprintIndices)
    {
        FIntPoint Index(Footprint % GridSize.X, Footprint / GridSize.X);
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                GetWorldLocationFromGridIndex(Index + FIntPoint(i, j), FRotator::ZeroRotator, AddedLocation);
                LocationsToCheck.Add(AddedLocation);
            }
        }    
    }

    GetCellCenterWorldLocationFromGridIndex(PivotPointIndex, AddedLocation);
    LocationsToCheck.Add(AddedLocation);

    float LowestHeight = MAX_FLT;
    for (const FVector& CheckedLocation: LocationsToCheck)
    {
        // PERFORM THE FIRST RAYCAST TO FIND THE FLOOR BELOW THE GRID ACTOR
        // Cell center location is already based on the grid actor's location, so we can directly raycast downwards from it.
        FVector Start = CheckedLocation;
        // I use 10000 here as a hugemongous number, shouldnt affect performance too much
        FVector End = CheckedLocation - FVector{ 0.f, 0.f, 10000.f };

        FHitResult Hit;
        bool bHit = false;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this); // Ignore the grid itself

        bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GridFloorChannel, Params);
        if (!bHit)
        {
            // There is no floor below the grid, this should not happen in a normal level, but just in case, we will return false to prevent tower placement.
            UE_LOG(LogTemp, Warning, TEXT("Somehow there is no floor below the grid. This happened at %s. Check that the ground is set to block the channel assigned to the floor."), *AddedLocation.ToString());
            return false;
        }

        if (Hit.Location.Z < LowestHeight)
        {
            LowestHeight = Hit.Location.Z;
        }
    }

    FVector OutLocation;
    GetWorldLocationFromGridIndex(PivotPointIndex, Rotation, OutLocation);
    OutLocation.Z = LowestHeight;
    
    OutTransform.SetLocation(OutLocation);
    
    FQuat FinalRotation = Rotation.Quaternion() * GetActorRotation().Quaternion();
    OutTransform.SetRotation(FinalRotation);
    OutTransform.SetScale3D(FVector(1.f, 1.f, 1.f));
    return true;
}



void AGridActor::GetTowerGridIndices(const FIntPoint& PivotPointIndex, const FRotator& Rotation, const FTowerData& TowerData, TArray<int32>& OutFootprintIndices, TArray<int32>& OutBoundaryIndices) const
{
    // Reserve memory to avoid re-allocations during the loop
    OutFootprintIndices.Empty(TowerData.Footprint.Num());
    OutBoundaryIndices.Empty(TowerData.Boundary.Num());
    FIntPoint CornerGridIndex = PivotPointIndex - RotateOffset(TowerData.PivotPoint, Rotation);

    // Helper to process the math once
    auto GetIndexForOffset = [&](const FIntPoint& Offset) -> int32
        {
            FIntPoint TargetIndex = CornerGridIndex + RotateOffset(Offset, Rotation);

            if (TargetIndex.X < 0 || TargetIndex.X >= GridSize.X ||
                TargetIndex.Y < 0 || TargetIndex.Y >= GridSize.Y)
            {
                return -1;
            }
            return (TargetIndex.Y * GridSize.X) + TargetIndex.X;
        };

    for (const FIntPoint& Offset : TowerData.Footprint)
    {
        OutFootprintIndices.Add(GetIndexForOffset(Offset));
    }

    for (const FIntPoint& Offset : TowerData.Boundary)
    {
        OutBoundaryIndices.Add(GetIndexForOffset(Offset));
    }
}

FIntPoint AGridActor::RotateOffset(const FIntPoint& Offset, const FRotator& Rotation) const
{
    FVector VectorOffset = FVector{ float(Offset.X), float(Offset.Y), 0.f };
    FVector RotatedVectorOffset = Rotation.RotateVector(VectorOffset);
    FIntPoint RotatedVectorOffsetInt = FIntPoint{ FMath::RoundToInt32(RotatedVectorOffset.X), FMath::RoundToInt32(RotatedVectorOffset.Y) };
    return RotatedVectorOffsetInt;
}

bool AGridActor::CanPlaceTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, FTowerData TowerData)
{
    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }

	TArray<int32> FootprintIndices;
	TArray<int32> BoundaryIndices;
	GetTowerGridIndices(PivotPointIndex, Rotation, TowerData, FootprintIndices, BoundaryIndices);
    
    // The tower's footprint must not be occupied by towers nor boundary
    for (const int32& Index: FootprintIndices)
    {
        if (!Occupied.IsValidIndex(Index))
        {
            return false; // Out of bounds
        }

        if (Occupied[Index] || BoundaryOccupied[Index] != 0)
        {
            return false; // Already filled or occupied by another tower's boundary
        }
    }

	for (const int32& Index: BoundaryIndices)
    {
        if (!BoundaryOccupied.IsValidIndex(Index))
        {
            continue; // Out of bounds, but it's okay for boundary
        }
        if (Occupied[Index])
        {
			// If the boundary overlaps with another tower, it's not allowed
            return false;
        }
    }

    return true;
}

bool AGridActor::PlaceTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, ATowerActor* TowerPtr)
{
    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
		UE_LOG(LogTemp, Warning, TEXT("Currently only supports rotation around Z axis"))
        return false;
    }

	if (!TowerPtr)
    {
		UE_LOG(LogTemp, Warning, TEXT("TowerPtr is null!"))
        return false;
    }

    TArray<int32> FootprintIndices;
    TArray<int32> BoundaryIndices;
    GetTowerGridIndices(PivotPointIndex, Rotation, TowerPtr->TowerData, FootprintIndices, BoundaryIndices);

    for (const int32& Index : FootprintIndices)
    {
        if (!Occupied.IsValidIndex(Index))
        {
            UE_LOG(LogTemp, Warning, TEXT("VERY SERIOUS: Footprint index out of bounds!"));
            // IF THIS HAPPENS, MEANS CALLER DID NOT CHECK BOUNDS BEFORE PLACING TOWER.
            continue;
		}
        Occupied[Index] = true;
        Towers[Index] = TowerPtr;
    }

	for (const int32& Index : BoundaryIndices)
    {
        if (!BoundaryOccupied.IsValidIndex(Index))
        {
            continue; // Out of bounds, but it's okay for boundary
        }
        BoundaryOccupied[Index] += 1;
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
    TArray<int32> FootprintIndices;
    TArray<int32> BoundaryIndices;
    GetTowerGridIndices(PivotPointIndex, Rotation, TowerPtr->TowerData, FootprintIndices, BoundaryIndices);

    for (const int32& Index : FootprintIndices)
    {
        if (!Occupied.IsValidIndex(Index))
        {
            UE_LOG(LogTemp, Warning, TEXT("VERY SERIOUS: Footprint index out of bounds!"));
            // IF THIS HAPPENS, MEANS CALLER DID NOT CHECK BOUNDS BEFORE PLACING TOWER.
            continue;
        }

        Occupied[Index] = false;
        Towers[Index] = nullptr;
    }
    for (const int32& Index : BoundaryIndices)
    {
        if (!BoundaryOccupied.IsValidIndex(Index))
        {
            continue; // Out of bounds, but it's okay for boundary
        }
        BoundaryOccupied[Index] -= 1;
    }
    return true;
}

void AGridActor::LogGridState()
{
    FString Output = "=\n";
    for (int Y = 0; Y < GridSize.Y; Y++)
    {
		for (int X = 0; X < GridSize.X; X++)
        {
            int CalculatedIndex = Y * GridSize.X + X;
            if (Occupied.IsValidIndex(CalculatedIndex) && Occupied[CalculatedIndex])
            {
                Output += "X";
            }
            else
            {
				Output += FString::FromInt(BoundaryOccupied[CalculatedIndex]);
            }
            if (X == GridSize.X - 1)
            {
                Output += "\n";
            }
            else
            {
                Output += " ";
            }
        }
    }
	UE_LOG(LogTemp, Log, TEXT("%s"), *Output);
}



