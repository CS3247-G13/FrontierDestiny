// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModeComponent.h"
#include "PlayerData.h"
#include "MassEnemyTarget.h"

#include "Upgrade.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UInputAction;
class UEnemyManagerSubsystem;

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
	void OnPlayerStatsUpdated();

	UFUNCTION()
	void OnRestoreBullets(int32 Amount);

	void UpdateBulletReplenishTimer();

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnWeaponChange OnWeaponChange;
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCurrentBulletsChange OnCurrentBulletsChange;

	void ReplenishBullet();

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> PrimaryFireAction;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> SecondaryFireAction;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> SwapWeaponAction;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> SelectWeaponAction;

	UFUNCTION()
	void OnFireAction(EWeaponType WeaponType);
	UFUNCTION()
	void PlayFireAnimation(EWeaponType WeaponType);
	UFUNCTION()
	void OnFireActionStart(EWeaponType WeaponType);

	// Code duplication intensifies
	void Recoil();
	void RecoverRecoil(float DeltaTime);
	void Shoot();
	void ShootDirection(FVector Direction);
	void ApplyHit(const FHitResult& Hit);
	void ApplyRifleUpgrades(UEnemyManagerSubsystem* EnemyManager, FMassEnemyTarget Target);
	void ApplyShotgunUpgrades(UEnemyManagerSubsystem* EnemyManager, FMassEnemyTarget Target, const FHitResult& Hit);
	bool TryConsumeBullets();
	void PlayTriggerSound();
	void PutOnCooldown();

	UPROPERTY(EditAnywhere, Category = "Setup")
	TEnumAsByte<ECollisionChannel> ShootingTargetChannel = ECC_Pawn;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	EWeaponType CurrentWeapon = EWeaponType::Rifle;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	int32 CurrentBullets;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float Cooldown;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector2D CurrentOffset;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FTimerHandle ReplenishBulletTimerHandle;

	UPROPERTY()
	UParticleSystemComponent* MuzzleFlashShotgun;

	UPROPERTY()
	UParticleSystemComponent* MuzzleFlashSniper;

	UFUNCTION(BlueprintCallable)
	void SetMuzzleFlashes(UParticleSystemComponent* Shotgun, UParticleSystemComponent* Sniper)
	{
		MuzzleFlashShotgun = Shotgun;
		MuzzleFlashSniper = Sniper;
	}
};