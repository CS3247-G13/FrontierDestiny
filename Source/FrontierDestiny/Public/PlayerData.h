// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 DamageAdded = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float DamageMultiplier = 1.f;

	int32 GetDamage()
	{
		return (Damage * DamageMultiplier) + DamageAdded;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRate = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRateAdded = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRateMultiplier = 1.f;

	float GetFireRate()
	{
		return (FireRate * FireRateMultiplier) + FireRateAdded;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Spread = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder")
	float BuildRange = 10000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MovementSpeed = 600.f;
};
