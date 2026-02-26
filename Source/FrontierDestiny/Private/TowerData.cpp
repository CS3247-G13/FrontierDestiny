// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerData.h"

FTowerStats FTowerStats::operator+(const FTowerStats& Other) const
{
    FTowerStats Result = *this;

    // Additive Stats
    Result.DamageAdded += Other.DamageAdded;
    Result.RangeAdded += Other.RangeAdded;
    Result.HealthAdded += Other.HealthAdded;
    Result.CooldownReduction += Other.CooldownReduction;

    Result.DamageMultiplier += Other.DamageMultiplier;
    Result.RangeMultiplier += Other.RangeMultiplier;
    Result.HealthMultiplier += Other.HealthMultiplier;
    Result.CooldownMultiplier += Other.CooldownMultiplier;

    // Handle Special Properties Map
    for (auto& Elem : Other.SpecialProperties)
    {
        if (Result.SpecialProperties.Contains(Elem.Key))
        {
            // Take the higher number if both have the key
            Result.SpecialProperties[Elem.Key] = FMath::Max(Result.SpecialProperties[Elem.Key], Elem.Value);
        }
        else
        {
            // Add if it doesn't exist
            Result.SpecialProperties.Add(Elem.Key, Elem.Value);
        }
    }

    return Result;
}