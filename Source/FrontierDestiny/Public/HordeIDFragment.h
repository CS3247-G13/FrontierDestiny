// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "HordeIDFragment.generated.h"

/** Stamped onto Mass entities at spawn time to track which horde they belong to. */
USTRUCT()
struct FRONTIERDESTINY_API FHordeIDFragment : public FMassFragment
{
	GENERATED_BODY()

	FName HordeID;
};
