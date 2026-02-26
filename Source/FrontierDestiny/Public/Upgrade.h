// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Upgrade.generated.h"

UENUM(BlueprintType)
enum class EUpgradeProperty : uint8
{
	// WEAPON
	WeaponFireRateAdded					UMETA(DisplayName = "Weapon Fire rate added"),
	WeaponFireRateMultiplier			UMETA(DisplayName = "Weapon Fire rate multiplier"),
	WeaponDamageAdded					UMETA(DisplayName = "Weapon Damage added"),
	WeaponDamageMultiplier				UMETA(DisplayName = "Weapon Damage multiplier"),
	WeaponSpreadReduction				UMETA(DisplayName = "Weapon Spread reduced"),

	// PLAYER
	PlayerAmmoReplenishRateAdded		UMETA(DisplayName = "Player Ammo Recharge rate added"),
	PlayerAmmoReplenishRateMultiplier	UMETA(DisplayName = "Player Ammo Recharge rate multiplier"),
	PlayerHealthAdded					UMETA(DisplayName = "Player Health added"),
	PlayerHealthMultiplier				UMETA(DisplayName = "Player Health multiplier"),
	PlayerMovementSpeedAdded			UMETA(DisplayName = "Player Movement speed added"),
	PlayerMovementSpeedMultiplier		UMETA(DisplayName = "Player Movement speed multiplier"),

	// TOWER
	TowerDamageAdded					UMETA(DisplayName = "Tower Damage added"),
	TowerDamageMultiplier				UMETA(DisplayName = "Tower Damage multiplier"),
	TowerCooldownReduction				UMETA(DisplayName = "Tower Cooldown reduction"),
	TowerCooldownMultiplier				UMETA(DisplayName = "Tower Cooldown multiplier"),
	TowerHealthAdded					UMETA(DisplayName = "Tower Health added"),
	TowerHealthMultiplier				UMETA(DisplayName = "Tower Health multiplier"),
	TowerRangeAdded						UMETA(DisplayName = "Tower Range added"),
	TowerRangeMultiplier				UMETA(DisplayName = "Tower Range multiplier"),
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FUpgradeDataRow : public FTableRowBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Name = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Description = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FName TargetID = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	int32 Cost = 0;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Special")
	TMap<EUpgradeProperty, float> Properties;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> PrerequisiteUpgradeIDs;
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FUpgradeData
{
	GENERATED_BODY()
public:
	FUpgradeData()
	{}

	FUpgradeData(const FUpgradeDataRow& Data):
		Name(Data.Name),
		Description(Data.Description),
		TargetID(Data.TargetID),
		Icon(Data.Icon),
		Cost(Data.Cost),
		Properties(Data.Properties),
		Prerequisites(Data.PrerequisiteUpgradeIDs),
		RemainingPrerequisites(Data.PrerequisiteUpgradeIDs)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Name = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Description = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FName TargetID = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	int32 Cost = 0;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Special")
	TMap<EUpgradeProperty, float> Properties;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> Prerequisites;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> Unlocks;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> RemainingPrerequisites;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	bool bIsCompleted = false;

	void AddUnlock(FName UpgradeID)
	{
		Unlocks.Add(UpgradeID);
	}

	void ClearPrerequisite(FName UpgradeID)
	{
		RemainingPrerequisites.Remove(UpgradeID);
	}

	void CompleteUpgrade()
	{
		bIsCompleted = true;
	}

	bool IsUnlocked()
	{
		return RemainingPrerequisites.IsEmpty();
	}
};