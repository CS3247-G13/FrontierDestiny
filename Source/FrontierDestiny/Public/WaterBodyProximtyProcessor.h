// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"

// Temporary water stuff
//#include "../../../../../../../../Program Files/Epic Games/UE_5.7/Engine/Plugins/Experimental/Water/Source/Runtime/Public/WaterSubsystem.h"
#include "WaterBodyActor.h"      // For AWaterBody
#include "WaterBodyComponent.h"  // For UWaterBodyComponent
#include "WaterSubsystem.h"      // If you use the subsystem
#include "Kismet/GameplayStatics.h"
#include "LakeData.h"

#include "WaterBodyProximtyProcessor.generated.h"



UCLASS()
class FRONTIERDESTINY_API UWaterBodyProximtyProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UWaterBodyProximtyProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;

private:
	FMassEntityQuery EntityQuery;

	//temporary water testing
	UPROPERTY()
	TArray<TObjectPtr<UWaterBodyComponent>> CachedWaterBodies;
	// Track if we have already scanned to avoid redundant searches
	bool bWaterCacheInitialized = false;

	TArray<FLakeData> CachedLakes; // runtime-only
};
