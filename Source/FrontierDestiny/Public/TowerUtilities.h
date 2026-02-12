// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TowerUtilities.generated.h"

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API UTowerUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "QuaternionInterp")
	static FRotator QuaternionSlerp(FRotator From, FRotator To, float DeltaTime, float RotationalSpeed);
};
