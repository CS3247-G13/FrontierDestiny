// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GlobalTowerSettings.generated.h"

class UMaterialInterface;
class UNiagaraSystem;

/**
 * Global settings for the Tower Defense system.
 * These will appear in Project Settings > Project > Global Tower Settings.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Global Tower Settings"))
class FRONTIERDESTINY_API UGlobalTowerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGlobalTowerSettings();

	/** The material applied to towers when they are in a valid ghost placement state. */
	UPROPERTY(Config, EditAnywhere, Category = "Ghost Visuals")
	TSoftObjectPtr<UMaterialInterface> GhostMaterialValid;

	/** The material applied to towers when they are in an invalid ghost placement state. */
	UPROPERTY(Config, EditAnywhere, Category = "Ghost Visuals")
	TSoftObjectPtr<UMaterialInterface> GhostMaterialInvalid;

	UPROPERTY(Config, EditAnywhere, Category = "Tower Data")
	TSoftObjectPtr<UDataTable> TowerDataTable;

	UPROPERTY(Config, EditAnywhere, Category = "Upgrades")
	TSoftObjectPtr<UDataTable> UpgradeDataTable;

	UPROPERTY(Config, EditAnywhere, Category = "Upgrades")
	TSoftObjectPtr<UDataTable> PlayerDataTable;

	UPROPERTY(Config, EditAnywhere, Category = "Upgrades")
	TSoftObjectPtr<UDataTable> EnemyDataTable;

	UPROPERTY(Config, EditAnywhere, Category = "Quest")
	TSoftObjectPtr<UDataTable> QuestDataTable;

	UPROPERTY(Config, EditAnywhere, Category = "Waves")
	TSoftObjectPtr<UDataTable> WaveDataTable;

	UPROPERTY(Config, EditAnywhere, Category = "Waves")
	TSoftObjectPtr<UDataTable> HordeDataTable;

	UPROPERTY(Config, EditAnywhere, Category = "Effects")
	TSoftObjectPtr<UNiagaraSystem> BloodSplatterEffect;


	/** Helper to get the settings instance easily */
	static const UGlobalTowerSettings* Get() { return GetDefault<UGlobalTowerSettings>(); }
};