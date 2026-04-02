// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CacheManagerSubsystem.generated.h"

UCLASS()
class FRONTIERDESTINY_API UCacheManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cache")
	void RegisterCache(AActor* Cache);

	UFUNCTION(BlueprintCallable, Category = "Cache")
	void DeregisterCache(AActor* Cache);

	UFUNCTION(BlueprintCallable, Category = "Cache")
	TArray<FVector> GetCachesInRange(FVector PlayerPos, float Radius) const;

	/** Enables custom stencil (37) on caches in range if bEnabled, clears all otherwise. */
	UFUNCTION(BlueprintCallable, Category = "Cache")
	void UpdateCacheHighlights(FVector PlayerPos, float Radius, bool bEnabled);

private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Caches;
};
