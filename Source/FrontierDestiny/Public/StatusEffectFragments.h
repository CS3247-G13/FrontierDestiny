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
	UPROPERTY(EditAnywhere) float DamagePerTick = 1.f;
	UPROPERTY(EditAnywhere) float TickInterval = 1.f;
	UPROPERTY(EditAnywhere) float TimeToNextTick = 1.f;
};

/** Remaining duration of a slow effect. Removed by StatusEffectProcessor when expired. */
USTRUCT()
struct FSlowFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float Duration = 0.f;
	/** Speed multiplier while slowed (e.g. 0.5 = 50% speed). */
	UPROPERTY(EditAnywhere) float SpeedMultiplier = 0.5f;
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

/** Pyroclastic shield — blocks one hit, then recharges over ChargeTime seconds. */
USTRUCT()
struct FPyroclasticFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float ChargeTime    = 5.f;
	UPROPERTY(EditAnywhere) float TimeToShield  = 0.f; // 0 = shield active
};

/** Base stats set at spawn time. Read by StatusEffectProcessor to calculate modified movement speed. */
USTRUCT()
struct FStatsFragment : public FMassFragment
{
	GENERATED_BODY()
	/** Current effective base speed — may be boosted by distortion. StatusEffectProcessor reads this. */
	UPROPERTY(EditAnywhere) float BaseSpeed = 1.f;
	/** Original speed set at spawn. Used by distortion to compute the boost. Never modified after spawn. */
	UPROPERTY(EditAnywhere) float InitialBaseSpeed = 1.f;
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

	/** Damage resistance multipliers — 1.0 = no resistance, 0.5 = 50% damage taken. */
	UPROPERTY(EditAnywhere, Category = "Resistance") float KineticResistance  = 1.f;
	UPROPERTY(EditAnywhere, Category = "Resistance") float LaserResistance    = 1.f;
	UPROPERTY(EditAnywhere, Category = "Resistance") float ElectricResistance = 1.f;
	UPROPERTY(EditAnywhere, Category = "Resistance") bool bArmoured    = false;
	UPROPERTY(EditAnywhere, Category = "Resistance") bool bReflective  = false;
	UPROPERTY(EditAnywhere, Category = "Resistance") bool bInsulated   = false;

	// Terrain modifiers (mutually exclusive — only one should be true per entity)
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bStealthy = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bNimble = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bPyroclastic = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bDistorted = false;
	/** Speed multiplier at full health depletion for distorted enemies (e.g. 1.5 = 50% faster at 0 HP). */
	UPROPERTY(EditAnywhere, Category = "Terrain") float DistortionSpeedMultiplier = 1.5f;

	/** Amorphic: max damage taken per hit. 0 = inactive. */
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bAmorphic = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") float AmorphicDamageCap  = 0.f;
	/** Fragmented: damage is floored to the nearest multiple of this value. 0 = inactive. */
	UPROPERTY(EditAnywhere, Category = "Terrain") bool bFragmented = false;
	UPROPERTY(EditAnywhere, Category = "Terrain") float FragmentedChunkSize = 0.f;
};
