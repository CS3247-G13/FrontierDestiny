// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ResourceAmount.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerManagerSubsystem.h"
#include "EconomySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFundsChanged, FResourceAmount, NewBalance, FResourceAmount, Delta);

UCLASS()
class FRONTIERDESTINY_API UEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool HasSufficientFunds(const FResourceAmount& Cost) const;

	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool TryDeductFunds(const FResourceAmount& Cost);

	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddFunds(const FResourceAmount& Amount);

	UFUNCTION(BlueprintCallable, Category = "Economy")
	FResourceAmount GetCurrentFunds() const { return CurrentFunds; }

	UPROPERTY(BlueprintAssignable, Category = "Economy")
	FOnFundsChanged OnFundsChanged;

	UFUNCTION(BlueprintCallable)
	void OnLevelChanged(UWorld* World);

protected:
	UPROPERTY(EditAnywhere, Category = "Economy")
	FResourceAmount CurrentFunds;

	UPROPERTY()
	FTimerHandle TimerHandle;

	UPROPERTY()
	TObjectPtr<UPlayerManagerSubsystem> PlayerManager;

	UFUNCTION()
	void AddPassiveIncome();
};
