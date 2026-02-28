// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyModifier.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class FRONTIERDESTINY_API UEnemyModifier : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FName TargetStatName;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float Multiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float Adder = 0.0f;

};
