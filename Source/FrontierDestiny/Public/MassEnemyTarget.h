// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "MassEnemyTarget.generated.h"

/** Wraps a Mass entity handle with its last-known world position for use in tower targeting. */
USTRUCT(BlueprintType)
struct FMassEnemyTarget
{
	GENERATED_BODY()

	// Not exposed as UPROPERTY — handle is opaque to Blueprints.
	// Use EnemyManagerSubsystem::ApplyDamageToTarget to act on this target from Blueprint.
	FMassEntityHandle EntityHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FVector Position = FVector::ZeroVector;

	FName EnemyID;

	bool IsSet() const { return EntityHandle.IsSet(); }
};
