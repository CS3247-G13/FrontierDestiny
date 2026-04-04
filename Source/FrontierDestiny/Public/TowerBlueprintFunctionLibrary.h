// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassEnemyTarget.h"
#include "Components/SceneComponent.h"
#include "TowerBlueprintFunctionLibrary.generated.h"

UCLASS()
class FRONTIERDESTINY_API UTowerBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Converts an array of hit results to unique FMassEnemyTarget entries, discarding hits that
	 *  aren't on Mass enemies and deduplicating by entity handle via IsSameTarget. */
	UFUNCTION(BlueprintCallable, Category = "Tower", meta = (WorldContext = "WorldContextObject"))
	static TArray<FMassEnemyTarget> GetUniqueEnemyTargetsFromHits(
		const UObject* WorldContextObject,
		const TArray<FHitResult>& HitResults,
		const TArray<FMassEnemyTarget>& IgnoreTargets);

	/** Single line trace on the Laser channel from Point to the target's last-known position.
	 *  Returns true if the first hit resolves to the given enemy. */
	UFUNCTION(BlueprintCallable, Category = "Tower", meta = (WorldContext = "WorldContextObject"))
	static bool CheckEnemyVisibleFromPoint(
		const UObject* WorldContextObject,
		FVector Point,
		const FMassEnemyTarget& Target,
		const TArray<AActor*>& IgnoreActors);

	/** Spawns the arc lightning Niagara effect between Origin and Target using the asset from GlobalTowerSettings. */
	UFUNCTION(BlueprintCallable, Category = "Tower", meta = (WorldContext = "WorldContextObject"))
	static void SpawnArcLightningEffect(
		const UObject* WorldContextObject,
		FVector Origin,
		FVector Target);

	/** Rotates TowerJoint (yaw) and TowerHead (pitch) to face TargetTransform using quaternion slerp. */
	UFUNCTION(BlueprintCallable, Category = "Tower")
	static void RotateTowerToFace(
		USceneComponent* TowerJoint,
		USceneComponent* TowerHead,
		FVector TargetPosition,
		float RotationSpeed,
		float DeltaTime);
};
