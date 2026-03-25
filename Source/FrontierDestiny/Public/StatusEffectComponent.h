// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusEffectComponent.generated.h"

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FStatusEffectTimer
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FTimerHandle TimerHandle;
	UPROPERTY()
	FTimerHandle ExpiryHandle;
	UPROPERTY()
	TObjectPtr<AController> Instigator;
	UPROPERTY()
	TObjectPtr<AActor> DamageCauser;
	UPROPERTY()
	float Damage = 0.f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FRONTIERDESTINY_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatusEffectComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ClearAllStatusEffects();

	UPROPERTY()
	TMap<FName, FStatusEffectTimer> ActiveDOTTimers;
public:	
	UFUNCTION(BlueprintCallable)
	void ApplyDOTByDamagePerTick(FName Tag, float DamagePerTick, float Duration, float Interval, AController* Instigator, AActor* DamageCauser);
	UFUNCTION(BlueprintCallable)
	void ApplyDOTByTotalDamage(FName Tag, float TotalDamage, float Duration, float Interval, AController* Instigator, AActor* DamageCauser);
	UFUNCTION(BlueprintCallable)
	void ApplyDOTByDamagePerSecond(FName Tag, float DamagePerSecond, float Duration, float Interval, AController* Instigator, AActor* DamageCauser);

	void DamageEntity(FName Tag);
	void HandleEffectExpired(FName Tag);
};
