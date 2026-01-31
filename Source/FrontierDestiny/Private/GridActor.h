// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridActor.generated.h"

class UBoxComponent;

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

	// Sets default values for this actor's properties
	AGridActor();

	/* 
	Takes a world hit result and returns a valid snapped grid position, and 2D index on the grid.
	Returns true if the location is valid.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Grid")
	bool GetSnappedGridIndex(const FHitResult& HitResult, FIntPoint& OutGridIndex) const;

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
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	virtual void OnConstruction(const FTransform& Transform) override;
private:

	
};
