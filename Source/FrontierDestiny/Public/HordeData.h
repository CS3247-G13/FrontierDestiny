// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ResourceAmount.h"
#include "HordeData.generated.h"

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FHordeDataRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Must match the HordeID used in the WaveDataTable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	FName HordeID;

	/** Resources awarded to the player when this horde is fully cleared. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	FResourceAmount Reward;
};
