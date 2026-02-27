// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerData.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridActor.generated.h"

class UBoxComponent;
class ATowerActor;

UCLASS()
class AGridActor : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float ScaleX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float ScaleY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float CellSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	FIntPoint GridSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Occupancy")
	TArray<bool> Occupied;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Occupancy")
	TArray<uint8> BoundaryOccupied;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Occupancy")
	TArray<TObjectPtr<ATowerActor>> Towers;
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

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Grid")
	void GetTowerGridIndices(const FIntPoint& PivotPointIndex, const FRotator& Rotation, const FTowerData& TowerData, TArray<int32>& OutFootprintIndices, TArray<int32>& OutBoundaryIndices) const;

	/*
	Get whether a tower can be placed at the pivot point
	*/
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool CanPlaceTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, FTowerData TowerData);

	/*
	Fills the cells based on tower
	*/
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool PlaceTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, ATowerActor* TowerPtr);

	/*
	Clears the cells based on tower
	*/
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool RemoveTower(const FIntPoint& PivotPointIndex, const FRotator& Rotation, ATowerActor* TowerPtr);

	UFUNCTION(BlueprintCallable)
	void LogGridState();
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	virtual void OnConstruction(const FTransform& Transform) override;

	FIntPoint RotateOffset(const FIntPoint& Offset, const FRotator& Rotation) const;
private:


};
