// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassEnemyTarget.h"
#include "TowerBlueprintFunctionLibrary.generated.h"

UCLASS()
class FRONTIERDESTINY_API UTowerBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/** Converts an array of hit results to unique FMassEnemyTarget entries, discarding hits that
	 *  aren't on Mass enemies and deduplicating by entity handle via IsSameTarget. */
	UFUNCTION(BlueprintCallable, Category = "Tower", meta = (WorldContext = "WorldContextObject"))
	static TArray<FMassEnemyTarget> GetUniqueEnemyTargetsFromHits(
		const UObject* WorldContextObject,
		const TArray<FHitResult>& HitResults,
		const TArray<FMassEnemyTarget>& IgnoreTargets);
};
