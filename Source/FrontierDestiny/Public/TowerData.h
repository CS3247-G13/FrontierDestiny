// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Upgrade.h"
#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "TowerData.generated.h"

class ATowerActor;

USTRUCT(BlueprintType)
struct FTowerEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText EffectName; // e.g., "Inner Blast", "Shockwave", "Lingering Fire"

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Amount = 0.0f;
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FTowerData : public FTableRowBase
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower")
	TSoftClassPtr<ATowerActor> Class = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	FString Name = "";

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	FString Description = "";

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Display")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Range = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	int32 Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Stats")
	float Cooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	TArray<FIntPoint> Footprint;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	TArray<FIntPoint> Boundary;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	FIntPoint PivotPoint = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Building")
	float Height = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Tower|Effects")
	TMap<FGameplayTag, FTowerEffect> Effects;

};
