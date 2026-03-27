// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "StatusEffectFragments.generated.h"

/** Remaining duration of a burn (DoT) effect. Removed by StatusEffectProcessor when expired. */
USTRUCT()
struct FBurnFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float Duration = 0.f;
};

/** Remaining duration of a slow effect. Removed by StatusEffectProcessor when expired. */
USTRUCT()
struct FSlowFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float Duration = 0.f;
};

/** Remaining duration of a stun effect. Removed by StatusEffectProcessor when expired. */
USTRUCT()
struct FStunFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float Duration = 0.f;
};

/** Additional HP pool (Vitality modifier). Drained before regular health by the damage processor. */
USTRUCT()
struct FVitalityFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float Value = 0.f;
	UPROPERTY(EditAnywhere) float MaxValue = 0.f;
};

/** Base stats set at spawn time. Read by StatusEffectProcessor to calculate modified movement speed. */
USTRUCT()
struct FStatsFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float BaseSpeed = 1.f;
	UPROPERTY(EditAnywhere) float BaseDamage = 10.f;
};

/** All modifier flags for an entity. Set once at spawn, never changed. */
USTRUCT()
struct FModifierFragment : public FMassFragment
{
	GENERATED_BODY()

	// Stackable modifiers
	UPROPERTY(EditAnywhere, Category = "Stackable") bool bFast = false;
	UPROPERTY(EditAnywhere, Category = "Stackable") bool bStrong = false;
	UPROPERTY(EditAnywhere, Category = "Stackable") bool bVitality = false;
	UPROPERTY(EditAnywhere, Category = "Stackable") bool bArmoured = false;
	UPROPERTY(EditAnywhere, Category = "Stackable") bool bReflective = false;
	UPROPERTY(EditAnywhere, Category = "Stackable") bool bInsulated = false;

	// Terrain modifiers (mutually exclusive — only one should be true per entity)
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bStealthy = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bNimble = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bPyroclastic = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bAmorphic = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bDistorted = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bFragmented = false;
};
