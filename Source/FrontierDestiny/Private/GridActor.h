// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerData.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridActor.generated.h"

class UBoxComponent;
class ATowerActor;

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FTowerPlacementIntent
{
	GENERATED_BODY()

	UPROPERTY()
	FTowerData TowerData;
	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	FIntPoint PivotPoint;
};

UCLASS()
class AGridActor : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TObjectPtr<UTexture2D> OccupancyTexture;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector OccupancyTextureSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float ScaleX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float ScaleY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float CellSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	FIntPoint GridSize;

	UPROPERTY(BlueprintReadWrite, Category = "Grid Occupancy")
	TArray<bool> Occupied;
	UPROPERTY(BlueprintReadWrite, Category = "Grid Occupancy")
	TArray<bool> BoundaryOccupied;

	// We perform a raycast from top to bottom to figure out where to place this
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	TEnumAsByte<ECollisionChannel> GridFloorChannel;

	// This just needs to be above the ground
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float RaycastOriginHeight;

	// Sets default values for this actor's properties
	AGridActor();

	/* 
	Takes a world location and returns a valid snapped grid position, and 2D index on the grid.
	Returns true if the location is valid.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool GetSnappedGridIndex(const FVector& HitLocation, FIntPoint& OutGridIndex) const;

	/*
	Takes a Grid Index and returns the world location at the center of the cell.
	By default, it returns the bottom-left corner of the cell.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	void GetCellCenterWorldLocationFromGridIndex(const FIntPoint& GridIndex, FVector& OutLocation) const;

	/*
	Takes a Grid Index and returns the world location at the corner of the cell defined by the Rotation.
	By default, it returns the bottom-left corner of the cell.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool GetWorldLocationFromGridIndex(const FIntPoint& GridIndex, const FRotator& Rotation, FVector& OutLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool GetTowerPlacementLocationFromGridIndex(const FTowerPlacementIntent& Placement, FTransform& OutTransform) const;

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Grid")
	void GetTowerGridIndices(const FTowerPlacementIntent& Placement, TArray<int32>& OutFootprintIndices, TArray<int32>& OutBoundaryIndices) const;

	/*
	Get whether a tower can be placed at the pivot point
	*/
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool CanPlaceTower(const FTowerPlacementIntent& Placement);

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	void ValidateTowerPlacements(TArray<FTowerPlacementIntent> Placements, TArray<bool>& OutValidPlacements);

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	void GetTowerPlacementsInLine(const FTowerPlacementIntent& Start, const FTowerPlacementIntent& End, int MaxTowers, TArray<FTowerPlacementIntent>& OutPlacements);

	/*
	Fills the cells based on tower
	*/
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool PlaceTower(const FTowerPlacementIntent& Placement);

	/*
	Clears the cells based on tower
	*/
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool RemoveTower(const FTowerPlacementIntent& Placement);
	UFUNCTION(BlueprintCallable)
	void UpdateOccupancyTexture(TArray<FTowerPlacementIntent> Placements);
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	virtual void OnConstruction(const FTransform& Transform) override;

	FIntPoint RotateOffset(const FIntPoint& Offset, const FRotator& Rotation) const;
private:


};
