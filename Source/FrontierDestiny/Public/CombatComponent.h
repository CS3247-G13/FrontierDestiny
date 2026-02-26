// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModeComponent.h"

#include "Upgrade.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UInputAction;

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
	EWeaponType Type;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 Damage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 DamageAdded;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float DamageMultiplier = 1.f;
	
	int32 GetDamage()
	{
		return (Damage * DamageMultiplier) + DamageAdded;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRateAdded;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRateMultiplier = 1.f;

	float GetFireRate()
	{
		return (FireRate * FireRateMultiplier) + FireRateAdded;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Spread;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SpreadReduction;

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
	float Range;
	
	UPROPERTY(EditAnywhere, Category = "Weapon")
	int32 AmmoCost;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<USoundBase> TriggerSound;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<USoundBase> OutOfBulletsSound;
};

// This is for changing the cross hair when the weapon changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChange, EWeaponType, Weapon);

// This is for updating the HUD with the number of bullets left
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrentBulletsChange, int32, CurrentBullets, int32, MaxBullets);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UCombatComponent : public UModeComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCombatComponent();

	virtual void SetupInput(UInputComponent* InputComponent) override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void ActivateMode() override;

protected:
	UFUNCTION()
	void OnUpgraded(const FUpgradeData& Upgrade);
	
	void UpdateBulletReplenishTimer();

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnWeaponChange OnWeaponChange;
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCurrentBulletsChange OnCurrentBulletsChange;

	void ReplenishBullet();

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> TriggerAction;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> SwapWeaponAction;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> SelectWeaponAction;

	UPROPERTY(EditAnywhere, Category = "Setup")
	float AmmoReplenishRate = 3.3333f;
	UPROPERTY(EditAnywhere, Category = "Setup")
	float AmmoReplenishRateAdded = 0.f;
	UPROPERTY(EditAnywhere, Category = "Setup")
	float AmmoReplenishRateMultiplier = 1.f;

	float GetAmmoReplenishRate()
	{
		return (AmmoReplenishRate * AmmoReplenishRateMultiplier) + AmmoReplenishRateAdded;
	}

	UFUNCTION()
	void OnTriggerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnTriggerStartAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnSwapWeaponAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnSelectWeaponAction(const FInputActionValue& Value);

	// Code duplication intensifies
	void Recoil();
	void RecoverRecoil(float DeltaTime);
	void Shoot();
	void ShootDirection(FVector Direction);
	bool TryConsumeBullets();
	void PlayTriggerSound();
	void PutOnCooldown();

	void SwapWeapon();
	void SelectWeapon(EWeaponType WeaponType);

	UPROPERTY(EditAnywhere, Category = "Setup")
	TEnumAsByte<ECollisionChannel> ShootingTargetChannel = ECC_Pawn;
	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 MaxBullets = 32;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TMap<EWeaponType, FWeaponData> WeaponDataMap;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	EWeaponType CurrentWeapon = EWeaponType::Rifle;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	int32 CurrentBullets;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TMap<EWeaponType, float> WeaponCooldowns;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector2D CurrentOffset;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FTimerHandle ReplenishBulletTimerHandle;
};