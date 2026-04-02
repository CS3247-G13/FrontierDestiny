// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreData.generated.h"

USTRUCT(BlueprintType)
struct FCoreData
{
	GENERATED_BODY()

public: 
	// Current HP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	float CurrentHP = 100.0f;

	// Maximum HP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	float MaximumHP = 100.0f;

	// Color of Core HP Bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	FLinearColor CoreColor = FLinearColor::Red;

	// Get HP Percentage
	float GetHPPercent() const
	{
		return MaximumHP > 0.f ? CurrentHP / MaximumHP : 0.f;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	UTexture2D* CoreIcon = nullptr;
};