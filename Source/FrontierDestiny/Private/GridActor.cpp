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
    BoundaryOccupied.Empty();
    Occupied.Init(false, GridSize.X * GridSize.Y);
	BoundaryOccupied.Init(false, GridSize.X * GridSize.Y);

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

bool AGridActor::GetTowerPlacementLocationFromGridIndex(const FTowerPlacementIntent& Placement, FTransform& OutTransform) const
{
    FIntPoint PivotPointIndex = Placement.PivotPoint;
    FRotator Rotation = Placement.Rotation;
    FTowerData TowerData = Placement.TowerData;

    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }
    TSet<FVector> LocationsToCheck;
    FVector AddedLocation;
    
    TArray<int32> FootprintIndices;
    TArray<int32> BoundaryIndices;
    GetTowerGridIndices(Placement, FootprintIndices, BoundaryIndices);

    for (const int32& Footprint : FootprintIndices)
    {
        FIntPoint Index(Footprint % GridSize.X, Footprint / GridSize.X);
        GetCellCenterWorldLocationFromGridIndex(Index, AddedLocation);
        LocationsToCheck.Add(AddedLocation);   
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
    FIntPoint CornerGridIndex = PivotPointIndex - RotateOffset(TowerData.PivotPoint, Rotation);

    GetWorldLocationFromGridIndex(CornerGridIndex, Rotation, OutLocation);
    OutLocation.Z = LowestHeight;
    
    OutTransform.SetLocation(OutLocation);
    
    FQuat FinalRotation = Rotation.Quaternion() * GetActorRotation().Quaternion();
    OutTransform.SetRotation(FinalRotation);
    OutTransform.SetScale3D(FVector(CellSize / 100.f));
    return true;
}

void AGridActor::GetTowerGridIndices(const FTowerPlacementIntent& Placement, TArray<int32>& OutFootprintIndices, TArray<int32>& OutBoundaryIndices) const
{
    FIntPoint PivotPointIndex = Placement.PivotPoint;
    FRotator Rotation = Placement.Rotation;
    FTowerData TowerData = Placement.TowerData;

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

void AGridActor::UpdateOccupancyTexture(TArray<FTowerPlacementIntent> Placements)
{
    // 1. Calculate Power of 2 dimensions
    int32 TexWidth = FMath::RoundUpToPowerOfTwo(GridSize.X);
    int32 TexHeight = FMath::RoundUpToPowerOfTwo(GridSize.Y);

    OccupancyTextureSize = FVector(TexWidth, TexHeight, 0.f);

    // 2. Create the transient texture
    // Using PF_B8G8R8A8 (Blue, Green, Red, Alpha)
    if (!OccupancyTexture || OccupancyTexture->GetSizeX() != TexWidth || OccupancyTexture->GetSizeY() != TexHeight)
    {
        OccupancyTexture = UTexture2D::CreateTransient(TexWidth, TexHeight, PF_B8G8R8A8);
        if (!OccupancyTexture) return;

        OccupancyTexture->CompressionSettings = TC_VectorDisplacementmap;
        OccupancyTexture->SRGB = false;
        OccupancyTexture->Filter = TF_Nearest;
        OccupancyTexture->UpdateResource();
    }

    // 3. Lock the texture for editing
    FTexture2DMipMap& Mip = OccupancyTexture->GetPlatformData()->Mips[0];
    void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
    uint8* RawData = (uint8*)Data;

    // Clear buffer to black/transparent
    FMemory::Memzero(RawData, TexWidth * TexHeight * 4);

    for (const FTowerPlacementIntent& Placement : Placements)
    {
        TArray<int32> CurrentFootprintIndices;
        TArray<int32> CurrentBoundaryIndices;
        GetTowerGridIndices(Placement, CurrentFootprintIndices, CurrentBoundaryIndices);

        for (int32 Index : CurrentFootprintIndices)
        {
            if (!Occupied.IsValidIndex(Index))
            {
                continue; // Out of bounds
            }

            // Convert 1D Grid Index back to 2D Grid Coordinates
            int32 x = Index % GridSize.X;
            int32 y = Index / GridSize.X;

            // Convert 2D Grid Coordinates to Texture Pixel Index
            int32 PixelIndex = (y * TexWidth + x) * 4;

            RawData[PixelIndex + 2] = 255; // R channel
        }

        // 6. Overlay Ghost Boundary (Alpha Channel)
        for (int32 Index : CurrentBoundaryIndices)
        {
            if (!BoundaryOccupied.IsValidIndex(Index))
            {
                continue; // Out of bounds
            }

            int32 x = Index % GridSize.X;
            int32 y = Index / GridSize.X;
            int32 PixelIndex = (y * TexWidth + x) * 4;

            RawData[PixelIndex + 3] = 255; // A channel
        }
    }

    for (int32 y = 0; y < GridSize.Y; y++)
    {
        for (int32 x = 0; x < GridSize.X; x++)
        {
            int32 GridIndex = y * GridSize.X + x;
            int32 PixelIndex = (y * TexWidth + x) * 4;

            // Logic: OccupiedFootprint = Blue Channel, OccupiedBoundary = Green Channel
            // CurrentFootprint = Red Channel, CurrentBoundary = Alpha Channel

            // Using 255 for full intensity
            if (Occupied.IsValidIndex(GridIndex) && Occupied[GridIndex])
            {
                RawData[PixelIndex + 0] = 255; // B (Blue for Footprint)
            }

            if (BoundaryOccupied.IsValidIndex(GridIndex) && BoundaryOccupied[GridIndex])
            {
                RawData[PixelIndex + 1] = 255; // G (Red for Boundary)
            }
        }
    }

    Mip.BulkData.Unlock();
    OccupancyTexture->UpdateResource();
}

bool AGridActor::CanPlaceTower(const FTowerPlacementIntent& Placement)
{
    FIntPoint PivotPointIndex = Placement.PivotPoint;
    FRotator Rotation = Placement.Rotation;
    FTowerData TowerData = Placement.TowerData;
    if (Rotation.Pitch != 0.f || Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }

	TArray<int32> FootprintIndices;
	TArray<int32> BoundaryIndices;
	GetTowerGridIndices(Placement, FootprintIndices, BoundaryIndices);
    
    // The tower's footprint must not be occupied by towers nor boundary
    for (const int32& Index: FootprintIndices)
    {
        if (!Occupied.IsValidIndex(Index))
        {
            return false; // Out of bounds
        }

        if (Occupied[Index] || BoundaryOccupied[Index])
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
        if (Occupied[Index] || BoundaryOccupied[Index])
        {
			// If the boundary overlaps with another tower, it's not allowed
            return false;
        }
    }

    // Check that every footprint cell sits on allowed terrain
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    for (const int32& Index : FootprintIndices)
    {
        if (Index < 0) return false;

        FIntPoint GridIndex(Index % GridSize.X, Index / GridSize.X);
        FVector CellCenter;
        GetCellCenterWorldLocationFromGridIndex(GridIndex, CellCenter);

        FHitResult Hit;
        FVector Start = CellCenter;
        FVector End = CellCenter - FVector(0.f, 0.f, 10000.f);
        bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);

        if (!bHit || !Hit.GetActor() || !Hit.GetActor()->ActorHasTag(AllowedTerrainTag))
        {
            return false;
        }
    }

    return true;
}

void AGridActor::ValidateTowerPlacements(TArray<FTowerPlacementIntent> Placements, TArray<bool>& OutValidPlacements)
{
    OutValidPlacements.Empty();
    OutValidPlacements.Init(false, Placements.Num());
    for (int i = 0; i < Placements.Num(); i++)
    {
        OutValidPlacements[i] = CanPlaceTower(Placements[i]);
    }
}

void AGridActor::GetTowerPlacementsInLine(const FTowerPlacementIntent& Start, const FTowerPlacementIntent& End, int MaxTowers, TArray<FTowerPlacementIntent>& OutPlacements)
{
    FIntPoint UpperBounds{ INT_MIN, INT_MIN };
    FIntPoint LowerBounds{ INT_MAX, INT_MAX };
    TArray<FIntPoint> AllPoints = Start.TowerData.Footprint;
    AllPoints.Append(Start.TowerData.Boundary);

    for (const FIntPoint& Point : AllPoints)
    {
        UpperBounds.X = FMath::Max(Point.X, UpperBounds.X);
        UpperBounds.Y = FMath::Max(Point.Y, UpperBounds.Y);
        LowerBounds.X = FMath::Min(Point.X, LowerBounds.X);
        LowerBounds.Y = FMath::Min(Point.Y, LowerBounds.Y);
    }

    FIntPoint BoundingBoxSize = UpperBounds - LowerBounds + FIntPoint(1, 1);
    FIntPoint Delta = End.PivotPoint - Start.PivotPoint;
    
    float NormalizedYaw = FRotator::ClampAxis(Start.Rotation.Yaw);

    if (FMath::IsNearlyEqual(NormalizedYaw, 90.0f, 0.1f) ||
        FMath::IsNearlyEqual(NormalizedYaw, 270.0f, 0.1f))
    {
        int32 Temp = BoundingBoxSize.X;
        BoundingBoxSize.X = BoundingBoxSize.Y;
        BoundingBoxSize.Y = Temp;
    }

    OutPlacements.Empty();

    bool bXIsDriving = FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y);

    // 2. Assign values based on the driving axis
    int32 StartDrive = bXIsDriving ? Start.PivotPoint.X : Start.PivotPoint.Y;
    int32 EndDrive = bXIsDriving ? End.PivotPoint.X : End.PivotPoint.Y;
    int32 StartDep = bXIsDriving ? Start.PivotPoint.Y : Start.PivotPoint.X;

    // Determine step direction (1 or -1) and size
    int32 DriveStepSign = (EndDrive >= StartDrive) ? 1 : -1;
    int32 DriveStepSize = (bXIsDriving ? BoundingBoxSize.X : BoundingBoxSize.Y) * DriveStepSign;

    // Calculate the slope (Ratio) relative to the driving axis
    float Ratio = 0.0f;
    if (Delta.X != 0 && Delta.Y != 0)
    {
        Ratio = bXIsDriving ? (static_cast<float>(Delta.Y) / Delta.X) : (static_cast<float>(Delta.X) / Delta.Y);
    }

    // 3. Single Loop
    for (int i = 0; i < MaxTowers; i++)
    {
        // Calculate current offset on the driving axis
        int32 CurrentDriveOffset = i * DriveStepSize;
        float CurrentDrivePos = StartDrive + CurrentDriveOffset;

        // Check if we've overshot the target
        if ((DriveStepSign > 0 && CurrentDrivePos > EndDrive) ||
            (DriveStepSign < 0 && CurrentDrivePos < EndDrive))
        {
            break;
        }

        // Calculate the dependent position based on the slope
        float CurrentDepPos = StartDep + (CurrentDriveOffset * Ratio);

        FTowerPlacementIntent Placement = Start; // Copy properties like Rotation/Data
        Placement.PivotPoint.X = bXIsDriving ? FMath::RoundToInt(CurrentDrivePos) : FMath::RoundToInt(CurrentDepPos);
        Placement.PivotPoint.Y = bXIsDriving ? FMath::RoundToInt(CurrentDepPos) : FMath::RoundToInt(CurrentDrivePos);

        OutPlacements.Add(Placement);
    }
}

bool AGridActor::PlaceTower(const FTowerPlacementIntent& Placement)
{
    if (Placement.Rotation.Pitch != 0.f || Placement.Rotation.Roll != 0.f)
    {
		UE_LOG(LogTemp, Warning, TEXT("Currently only supports rotation around Z axis"))
        return false;
    }

    TArray<int32> FootprintIndices;
    TArray<int32> BoundaryIndices;

    GetTowerGridIndices(Placement, FootprintIndices, BoundaryIndices);

    for (const int32& Index : FootprintIndices)
    {
        if (!Occupied.IsValidIndex(Index))
        {
            UE_LOG(LogTemp, Warning, TEXT("VERY SERIOUS: Footprint index out of bounds!"));
            // IF THIS HAPPENS, MEANS CALLER DID NOT CHECK BOUNDS BEFORE PLACING TOWER.
            continue;
		}
        Occupied[Index] = true;
    }

	for (const int32& Index : BoundaryIndices)
    {
        if (!BoundaryOccupied.IsValidIndex(Index))
        {
            continue; // Out of bounds, but it's okay for boundary
        }
        BoundaryOccupied[Index] = true;
    }

	return true;
}

bool AGridActor::RemoveTower(const FTowerPlacementIntent& Placement)
{
    if (Placement.Rotation.Pitch != 0.f || Placement.Rotation.Roll != 0.f)
    {
        // Currently only supports rotation around Z axis
        return false;
    }
    TArray<int32> FootprintIndices;
    TArray<int32> BoundaryIndices;
    GetTowerGridIndices(Placement, FootprintIndices, BoundaryIndices);

    for (const int32& Index : FootprintIndices)
    {
        if (!Occupied.IsValidIndex(Index))
        {
            UE_LOG(LogTemp, Warning, TEXT("VERY SERIOUS: Footprint index out of bounds!"));
            // IF THIS HAPPENS, MEANS CALLER DID NOT CHECK BOUNDS BEFORE PLACING TOWER.
            continue;
        }

        Occupied[Index] = false;
    }
    for (const int32& Index : BoundaryIndices)
    {
        if (!BoundaryOccupied.IsValidIndex(Index))
        {
            continue; // Out of bounds, but it's okay for boundary
        }
        BoundaryOccupied[Index] = false;
    }
    return true;
}



