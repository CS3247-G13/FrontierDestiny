// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDied);

UCLASS()
class FRONTIERDESTINY_API AEnemyActor : public ACharacter
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemyActor();

	UPROPERTY(BlueprintAssignable, Category = "Enemy Events")
	FOnEnemyDied OnEnemyDied;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Properties")
	int32 MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Properties")
	int32 CurrentHealth;

	UFUNCTION(BlueprintCallable, Category = "Enemy Functions")
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;
};
