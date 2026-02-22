// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModeComponent.h"

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

protected:
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

	UPROPERTY(EditAnywhere, Category = Setup)
	float BulletReplenishCooldown = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Setup|Rifle")
	TObjectPtr<USoundBase> RifleTriggerSound;
	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	TObjectPtr<USoundBase> ShotgunTriggerSound;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<USoundBase> OutOfBulletsSound;

	UFUNCTION()
	void OnTriggerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnTriggerStartAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnSwapWeaponAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnSelectWeaponAction(const FInputActionValue& Value);

	// Code duplication intensifies
	void RifleRecoil();
	void RifleRecoverRecoil(float DeltaTime);

	void ShotgunRecoil();
	void ShotgunRecoverRecoil(float DeltaTime);

	void PerformRifleShoot();
	void PerformShotgunShoot();

	void SwapWeapon();
	void SelectWeapon(EWeaponType WeaponType);

	UPROPERTY(EditAnywhere, Category = "Setup")
	TEnumAsByte<ECollisionChannel> ShootingTargetChannel = ECC_Pawn;
	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 MaxBullets = 32;

	UPROPERTY(EditAnywhere, Category = "Setup|Rifle")
	float RifleFireRate = 3.333333333333333f;
	UPROPERTY(EditAnywhere, Category = "Setup|Rifle")
	FVector2D RifleRecoilAmount{ 0.0f, 0.4f };
	UPROPERTY(EditAnywhere, Category = "Setup|Rifle")
	FVector2D RifleMaxRecoilAmount{ 0.0f, 1.f };
	UPROPERTY(EditAnywhere, Category = "Setup|Rifle")
	FVector2D RifleOffsetRecoverySpeed{ 1.f, 3.f };
	UPROPERTY(EditAnywhere, Category = "Setup|Rifle")
	float RifleRange = 10000.f;
	UPROPERTY(EditAnywhere, Category = "Setup|Rifle")
	int32 RifleBulletCost = 1;

	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	float ShotgunFireRate = 1.f;
	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	FVector2D ShotgunRecoilAmount{ 0.0f, 5.f };
	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	FVector2D ShotgunMaxRecoilAmount{ 0.0f, 10.f };
	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	FVector2D ShotgunOffsetRecoverySpeed{ 0.f,};
	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	float ShotgunRange = 10000.f;
	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	float ShotgunSpreadAngle = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Setup|Shotgun")
	int32 ShotgunBulletCost = 9;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	EWeaponType CurrentWeapon = EWeaponType::Rifle;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	int32 CurrentBullets;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float CurrentCooldown = 0.f;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector2D CurrentOffset;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FTimerHandle ReplenishBulletTimerHandle;
};