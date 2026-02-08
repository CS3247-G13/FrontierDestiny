// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EconomyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFundsChanged, int32, NewBalance, int32, DeltaFunds);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FRONTIERDESTINY_API UEconomyComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEconomyComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool HasSufficientFunds(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool TryDeductFunds(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddFunds(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Economy")
	int32 GetCurrentFunds() const { return CurrentFunds; }

	UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category = "Economy")
	FOnFundsChanged OnFundsChanged;

protected:
	UPROPERTY(EditAnywhere, Category = "Economy")
	int32 CurrentFunds = 100;
	UPROPERTY(EditAnywhere, Category = "Economy")
	int32 PassiveIncomePerSecond = 5;

	UPROPERTY()
	FTimerHandle TimerHandle;
	UFUNCTION()
	void AddPassiveIncome();
};
