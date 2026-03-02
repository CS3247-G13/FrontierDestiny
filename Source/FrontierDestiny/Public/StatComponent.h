// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatComponent.generated.h"

//idk forward declare
class UEnemyModifier;

USTRUCT(BlueprintType)
struct FEntityAttribute {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float BaseValue;

	float CurrentMaxValue;

	float CurrentValue;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStatComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TMap<FName, FEntityAttribute> Attributes;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetStat(FName StatName) const {
		//return Attributes.Contains(StatName) ? Attributes[StatName].CurrentValue : 0.0f;
		if (Attributes.Contains(StatName)) {
			float Val = Attributes[StatName].CurrentValue;

			// This will print to your Output Log (Window -> Output Log)
			// UE_LOG(LogTemp, Warning, TEXT("Found Stat: %s with Value: %f"), *StatName.ToString(), Val);

			return Val;
		}

		// UE_LOG(LogTemp, Error, TEXT("Stat: %s NOT FOUND in Attributes"), *StatName.ToString());
		return 0.0f;
	}
	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetBaseStat(FName StatName) const {
		return Attributes.Contains(StatName) ? Attributes[StatName].BaseValue : 0.0f;
	}

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ApplyModifier(UEnemyModifier* Modifier);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddValue(FName StatName, float Amount);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	TArray<UEnemyModifier*> ActiveModifiers;

	void RecalculateStat(FName StatName);
};
