// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoreActor.h"
#include "CoreManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCapturedCoreCountChanged, int32, NewCount);

/**
 * Tracks all CoreActors in the current level.
 * Provides a single source of truth for how many cores are captured,
 * which drives the wave system's per-core-count spawn scaling.
 */
UCLASS()
class FRONTIERDESTINY_API UCoreManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Called by each CoreActor in BeginPlay to register itself. */
	void RegisterCore(ACoreActor* Core);

	/** Activates the core with the given index. No-op if the index doesn't exist. */
	UFUNCTION(BlueprintCallable)
	void ActivateCore(int32 CoreIndex);

	/** Returns the number of cores that are currently active (captured). */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetCapturedCoreCount() const;

	/** Returns the core actor for the given index, or nullptr if not found. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	ACoreActor* GetCore(int32 CoreIndex) const;

	/** Returns the world location of the nearest active core to FromLocation. Returns FVector::ZeroVector if no active cores exist. */
	FVector GetNearestActiveCoreLocation(FVector FromLocation) const;

	/** Returns the nearest active core actor to FromLocation, or nullptr if none exist. */
	ACoreActor* GetNearestActiveCore(FVector FromLocation) const;

	/** Broadcasts whenever the number of captured cores changes. */
	UPROPERTY(BlueprintAssignable)
	FOnCapturedCoreCountChanged OnCapturedCoreCountChanged;

private:
	/** Map of CoreIndex → CoreActor. Populated at BeginPlay by each CoreActor. */
	UPROPERTY()
	TMap<int32, ACoreActor*> CoreMap;

	int32 CapturedCoreCount = 0;

	UFUNCTION()
	void HandleCoreActivated(ACoreActor* Core);

	UFUNCTION()
	void HandleCoreDestroyed(ACoreActor* Core);
};
