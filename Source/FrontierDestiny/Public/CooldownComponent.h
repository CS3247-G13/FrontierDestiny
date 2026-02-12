// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CooldownComponent.generated.h"

/** Defines how the cooldown behaves during its duration */
UENUM(BlueprintType)
enum class ECooldownMode : uint8
{
	DeactivateOnFinish    UMETA(DisplayName = "Deactivate"),    // Stops ticking when finished
	LoopOnFinish		  UMETA(DisplayName = "Loop"),			// Restarts cooldown when finished
	TickOnFinish		  UMETA(DisplayName = "Tick")			// Continues ticking even when finished
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCooldownFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCooldownTick, float, ProgressPercent);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UCooldownComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCooldownComponent();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	float CooldownDuration;

	/** Start the cooldown with a specific duration */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void StartCooldown();

	/** Start the cooldown with a specific duration */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void PauseCooldown();

	/** Resets the cooldown to zero and stops it */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void ResetCooldown();

	/** Returns true if the cooldown is currently counting down */
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	bool IsOnCooldown() const { return bIsActive; }

	/** Remaining time in seconds */
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetRemainingTime() const { return FMath::Max(0.f, CooldownDuration - ElapsedTime); }

	/** 0.0 to 1.0 progress of the cooldown */
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetProgressPercent() const { return CooldownDuration > 0.f ? (ElapsedTime / CooldownDuration) : 1.f; }

	/** Multiplier for how fast time passes (1.0 is normal, 2.0 is double speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	float CooldownMultiplier = 1.0f;

	/** Called when the cooldown reaches 100% */
	UPROPERTY(BlueprintAssignable, Category = "Cooldown")
	FOnCooldownFinished OnCooldownFinished;

	/** Called every frame during the cooldown (only if Mode is Continuous) */
	UPROPERTY(BlueprintAssignable, Category = "Cooldown")
	FOnCooldownTick OnCooldownTick;

	/** Whether to fire events every frame or just at the end */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	ECooldownMode Mode = ECooldownMode::DeactivateOnFinish;

private:
	bool bIsActive = false;
	float ElapsedTime = 0.0f;
};