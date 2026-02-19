// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModeComponent.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UInputAction;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FRONTIERDESTINY_API UCombatComponent : public UModeComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	virtual void SetupInput(UInputComponent* InputComponent) override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> TriggerAction;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> SwapWeaponAction;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> SelectWeaponAction;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<USoundBase> TriggerSound;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<USoundBase> OutOfBulletsSound;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<USoundBase> ReloadSound;

	UFUNCTION()
	void OnTriggerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnTriggerStartAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnReloadAction(const FInputActionValue& Value);
	UFUNCTION()
	void Recoil();
	UFUNCTION()
	void RecoverRecoil(float DeltaTime);
	UFUNCTION()
	void PerformShoot();
	UFUNCTION()
	void PerformReload();

	UPROPERTY(EditAnywhere, Category = "Setup")
	TEnumAsByte<ECollisionChannel> ShootingTargetChannel = ECC_Pawn;
	UPROPERTY(EditAnywhere, Category = "Setup")
	float FireRate = 3.333333333333333f;
	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector2D RecoilAmount { 0.0f, 0.4f };
	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector2D MaxRecoilAmount { 0.0f, 1.f };
	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector2D OffsetRecoverySpeed { 1.f, 3.f };
	UPROPERTY(EditAnywhere, Category = "Setup")
	float Range = 10000.f;
	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 MaxBullets = 32;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	int32 CurrentBullets;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float CurrentCooldown = 0.f;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector2D CurrentOffset;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float FireDuration;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	bool bIsFiring = false;
};
