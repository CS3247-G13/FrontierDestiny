// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityConfigAsset.h"
#include "CoreMinimal.h"
#include "EnemyData.generated.h"

USTRUCT(BlueprintType)
struct FEntityAttribute {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float BaseValue;

	float CurrentMaxValue;

	float CurrentValue;
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FEnemyData : public FTableRowBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	TMap<FName, FEntityAttribute> Attributes;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMassEntityConfigAsset> EnemyMassEntityAsset;

	/** If true, this enemy type counts as enhanced — shown as a darker blip on the minimap when priority targeting is active. */
	UPROPERTY(EditAnywhere)
	bool bIsEnhanced = false;
};
