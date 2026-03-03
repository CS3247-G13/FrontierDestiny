// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemyCharacter.generated.h"

class UStatComponent;
class UEnemyModifier;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS()
class FRONTIERDESTINY_API ABaseEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Setup")
	FName EnemyID;

	// Sets default values for this character's properties
	ABaseEnemyCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	UStatComponent* StatComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<UEnemyModifier*> StartingModifiers;

	UPROPERTY(BlueprintAssignable)
	FOnDeath OnDeath;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};
