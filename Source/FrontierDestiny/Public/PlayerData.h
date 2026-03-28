// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ResourceAmount.h"
#include "PlayerData.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Rifle	UMETA(DisplayName = "Rifle"),
	Shotgun	UMETA(DisplayName = "Shotgun")
};



USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FWeaponData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 Damage = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 DamageAdded = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	float DamageMultiplier = 1.f;

	int32 GetDamage()
	{
		return (Damage * DamageMultiplier) + DamageAdded;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRate = 1.f;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	float FireRateAdded = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	float FireRateMultiplier = 1.f;

	float GetFireRate()
	{
		return (FireRate * FireRateMultiplier) + FireRateAdded;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Spread = 1.5f;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	float SpreadReduction = 0.f;

	float GetSpread()
	{
		return FMath::Max(0.01f, Spread - SpreadReduction);
	}

	UPROPERTY(EditAnywhere, Category = "Weapon")
	FVector2D RecoilAmount{ 0.0f, 0.4f };
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FVector2D MaxRecoilAmount{ 0.0f, 1.f };
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FVector2D OffsetRecoverySpeed{ 1.f, 3.f };

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float Range = 10000.f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	int32 AmmoCost = 1;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<USoundBase> TriggerSound;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<USoundBase> OutOfBulletsSound;
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FPlayerData : public FTableRowBase
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "Combat")
	TMap<EWeaponType, FWeaponData> WeaponDataMap;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxBullets = 32;

	UPROPERTY(EditAnywhere, Category = "Upgrade Values|Bullet Reservoir")
	int32 BulletReservoir1Amount = 8;
	UPROPERTY(EditAnywhere, Category = "Upgrade Values|Bullet Reservoir")
	int32 BulletReservoir2Amount = 8;
	UPROPERTY(EditAnywhere, Category = "Upgrade Values|Bullet Reservoir")
	int32 BulletReservoir3Amount = 8;

	int32 GetMaxBullets() const
	{
		int32 Bonus = 0;
		if (bBulletReservoir1) Bonus += BulletReservoir1Amount;
		if (bBulletReservoir2) Bonus += BulletReservoir2Amount;
		if (bBulletReservoir3) Bonus += BulletReservoir3Amount;
		return MaxBullets + Bonus;
	}

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AmmoReplenishRate = 3.3333f;
	UPROPERTY()
	float AmmoReplenishRateAdded = 0.f;
	UPROPERTY()
	float AmmoReplenishRateMultiplier = 1.f;

	float GetAmmoReplenishRate()
	{
		return (AmmoReplenishRate * AmmoReplenishRateMultiplier) + AmmoReplenishRateAdded;
	}

	UPROPERTY(EditAnywhere, Category = "Economy")
	FResourceAmount PassiveIncomePerSecond;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder")
	float BuildRange = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MovementSpeed = 600.f;

	// Inferno Cartridge
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Inferno Cartridge")
	float InfernoCartridgeDamagePerTick = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Inferno Cartridge")
	float InfernoCartridgeDuration = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Inferno Cartridge")
	float InfernoCartridgeTickInterval = 1.f;

	// Rupture Rounds
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Rupture Rounds")
	float RuptureRoundsDamage = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Rupture Rounds")
	float RuptureRoundsRadius = 1.f;

	// Suppressing Fire
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Suppressing Fire")
	int32 SuppressingFireShotCount = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Suppressing Fire")
	float SuppressingFireTimeWindow = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Suppressing Fire")
	float SuppressingFireSlowAmount = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Suppressing Fire")
	float SuppressingFireSlowDuration = 3.f;

	// Compounding Injury
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Compounding Injury")
	float CompoundingInjuryDamagePerStack = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Compounding Injury")
	float CompoundingInjuryMaxBonus = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Compounding Injury")
	float CompoundingInjuryResetTime = 2.f;

	// Arc Shots
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Arc Shots")
	float ArcShotRange = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Arc Shots")
	float ArcShotDamage = 5.f;

	// Conduit Marker
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Conduit Marker")
	float ConduitMarkerDuration = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Conduit Marker")
	float ConduitMarkerRange = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Conduit Marker")
	float ConduitMarkerArcDamage = 1.f;

	// Stagger Shells
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Stagger Shells")
	float StaggerShellsRange = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Stagger Shells")
	float StaggerShellsStunDuration = 2.f;

	// Ballistic Recall
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Ballistic Recall")
	float BallisticRecallRange = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Ballistic Recall")
	int32 BallisticRecallAmmoRegain = 4;

	// Slug Conversion
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Slug Conversion")
	float SlugConversionDamage = 60.f;

	// Devastating Blow
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Devastating Blow")
	float DevastatingBlowHPThreshold = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Values|Devastating Blow")
	float DevastatingBlowMultiplier = 1.5f;

	// Rifle upgrades
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Rifle")
	bool bPiercingShots = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Rifle")
	bool bRuptureRounds = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Rifle")
	bool bSuppressingFire = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Rifle")
	bool bCompoundingInjury = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Rifle")
	bool bArcShots = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Rifle")
	bool bConduitMarker = false;

	// Shotgun upgrades
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Shotgun")
	bool bInfernoCartridge = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Shotgun")
	bool bFlakBarrel = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Shotgun")
	bool bStaggerShells = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Shotgun")
	bool bBallisticRecall = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Shotgun")
	bool bSlugConversion = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Shotgun")
	bool bDevastatingBlow = false;

	// Ammo upgrades
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Ammo")
	bool bBulletReservoir1 = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Ammo")
	bool bBulletReservoir2 = false;
	UPROPERTY(BlueprintReadOnly, Category = "Upgrades|Ammo")
	bool bBulletReservoir3 = false;
};
