// Fill out your copyright notice in the Description page of Project Settings.


#include "GridActor.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

// Sets default values
AGridActor::AGridActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    // Default Variable Values
    ScaleX = 500.0f;
    ScaleY = 500.0f;
    CellSize = 100.0f;

    // 1. Create the Root
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // 2. Create the Visualizer Box
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("EditorVisualizer"));
    CollisionBox->SetupAttachment(Root);

    // Set default behavior for the visualizer
    CollisionBox->SetLineThickness(2.0f);
    CollisionBox->SetBoxExtent(FVector(ScaleX / 2.f, ScaleY / 2.f, 10.f));
    CollisionBox->bHiddenInGame = true; // Only see it in the Editor
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionProfileName(TEXT("GridCollision"));
	CollisionBox->SetRelativeLocation(FVector(ScaleX / 2.f, ScaleY / 2.f, 0.0f));

}

// THIS FUNCTION ASSUMES VERTICAL GRIDS ONLY, IF WE ROTATE THE GRID, UPDATE THIS FUNCTION
bool AGridActor::GetSnappedGridIndex(const FHitResult& HitResult, FIntPoint& OutGridIndex) const
{   
    FVector ActorLocation = GetActorLocation();
    OutGridIndex.X = FMath::Floor((HitResult.Location.X - ActorLocation.X) / CellSize);
    OutGridIndex.Y = FMath::Floor((HitResult.Location.Y - ActorLocation.Y) / CellSize);
	
    // Out of bounds
    if (OutGridIndex.X < 0 || OutGridIndex.X >= FMath::Floor(ScaleX / CellSize) ||
        OutGridIndex.Y < 0 || OutGridIndex.Y >= FMath::Floor(ScaleY / CellSize))
    {
        return false;
    }

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

void AGridActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

    if (CollisionBox)
    {
        CollisionBox->SetBoxExtent(FVector(ScaleX / 2.f, ScaleY / 2.f, 10.0f));
		CollisionBox->SetRelativeLocation(FVector(ScaleX / 2.f, ScaleY / 2.f, 0.0f));
    }
	
}


