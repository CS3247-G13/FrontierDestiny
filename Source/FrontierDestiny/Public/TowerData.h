// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Upgrade.h"
#include "CoreMinimal.h"
#include "TowerData.generated.h"

class ATowerActor;

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FTowerDataRow : public FTableRowBase
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower")
	TSoftClassPtr<ATowerActor> Class = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	FString Name = "";

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	FString Description = "";

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Damage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Range = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float Cooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	TArray<FIntPoint> Footprint;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	FIntPoint PivotPoint = FIntPoint(0, 0);

};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FTowerStats
{
	GENERATED_BODY()
public:
	FTowerStats()
		: Damage(0), DamageMultiplier(1.0f), DamageAdded(0)
		, Range(0), RangeAdded(0), RangeMultiplier(1.0f)
		, Health(0), HealthAdded(0), HealthMultiplier(1.0f)
		, Cooldown(1.0f), CooldownReduction(0.0f), CooldownMultiplier(1.0f)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Damage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 DamageAdded = 0;

	int32 GetDamage() const
	{
		return FMath::RoundToInt(Damage * DamageMultiplier + DamageAdded);
	}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Range = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 RangeAdded = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float RangeMultiplier = 1.f;

	int32 GetRange() const
	{
		return FMath::RoundToInt(Range * RangeMultiplier + RangeAdded);
	}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Health = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 HealthAdded = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float HealthMultiplier = 1.f;

	int32 GetHealth() const
	{
		return FMath::RoundToInt(Health * HealthMultiplier + HealthAdded);
	}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float Cooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float CooldownReduction = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float CooldownMultiplier = 1.f;

	float GetCooldown() const
	{
		return Cooldown * CooldownMultiplier - CooldownReduction;
	}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	TMap<EUpgradeProperty, float> SpecialProperties;

	float GetSpecialProperty(EUpgradeProperty PropertyName) const
	{
		if (const float* Value = SpecialProperties.Find(PropertyName))
		{
			return *Value;
		}
		return 0.0f;
	}

	FTowerStats operator+(const FTowerStats& Other) const;
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FTowerData
{
	GENERATED_BODY()

public:
	
	FTowerData()
	{
	}

	static FTowerData CreateTowerData(const FTowerDataRow& Data)
	{
		FTowerData Tower;

		Tower.Class = Data.Class;
		Tower.Name = Data.Name;
		Tower.Description = Data.Description;
		Tower.Icon = Data.Icon;
		Tower.Cost = Data.Cost;

		Tower.Stats.Damage = Data.Damage;
		Tower.Stats.Range = Data.Range;
		Tower.Stats.Cooldown = Data.Cooldown;
		Tower.Stats.Health = Data.Health;

		Tower.Footprint = Data.Footprint;
		Tower.PivotPoint = Data.PivotPoint;
		return Tower;
	}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower")
	TSoftClassPtr<ATowerActor> Class = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	FString Name = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	FString Description = "";

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	FTowerStats Stats;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	TArray<FIntPoint> Footprint;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	FIntPoint PivotPoint = FIntPoint(0, 0);
};