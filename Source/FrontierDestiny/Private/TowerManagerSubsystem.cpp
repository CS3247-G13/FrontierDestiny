// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerActor.h"
#include "GlobalTowerSettings.h"

#include "UpgradeSubsystem.h"
#include "TowerManagerSubsystem.h"

void UTowerManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency(UUpgradeManagerSubsystem::StaticClass());
	LoadTowerDataFromDataTable();

	// Register the callback to the upgrade performed event just so it is consistent with the player upgrade system
	UUpgradeManagerSubsystem* UpgradeManager = GetGameInstance()->GetSubsystem<UUpgradeManagerSubsystem>();
	UpgradeManager->OnUpgradePerformed.AddDynamic(this, &UTowerManagerSubsystem::UpgradeTower);
}

void UTowerManagerSubsystem::LoadTowerDataFromDataTable()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UDataTable* Table = Settings->TowerDataTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load Tower Stats Data Table (Make sure to set it in Project Settings). Proceeding with default tower stats..."));
		return;
	}

	const TMap<FName, uint8*>& RowMap = Table->GetRowMap();

	for (auto& Pair : RowMap)
	{
		FName RowName = Pair.Key;

		FTowerDataRow* Data = reinterpret_cast<FTowerDataRow*>(Pair.Value);

		if (Data)
		{
			FTowerData Stats = FTowerData::CreateTowerData(*Data);
			TowerDataMap.Add(RowName, Stats);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("TowerManager: Initialized %d towers."), TowerDataMap.Num());
}

void UTowerManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

}

void UTowerManagerSubsystem::UpgradeTower(const FUpgradeData& Upgrade)
{
	const FName& TowerID = Upgrade.TargetID;
	if (TowerID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade %s has no valid TowerTargetID. This should not happen! Make sure to set the TowerTargetID in the data table."), *Upgrade.Name);
		return;
	}
	if (!TowerDataMap.Contains(TowerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Tower ID %s not found in TowerDataMap. This should not happen! Make sure to add the tower stats to the data table. For now, this will initialize with default stats."), *TowerID.ToString());
		TowerDataMap.Add(TowerID, FTowerData());
	}

	// Apply the direct stats from the upgrade to the tower's stats
	// This adds the flat and multiplier values to each of the stats,
	// as well as applying any special effects that the upgrade grants.
	// These stats will have to be queried by the tower.

	TMap<EUpgradeProperty, float> Properties = Upgrade.Properties;

	const static TArray<EUpgradeProperty> StatProperties = {
		EUpgradeProperty::TowerDamageAdded,
		EUpgradeProperty::TowerDamageMultiplier,
		EUpgradeProperty::TowerCooldownReduction,
		EUpgradeProperty::TowerCooldownMultiplier,
		EUpgradeProperty::TowerRangeAdded,
		EUpgradeProperty::TowerRangeMultiplier,
		EUpgradeProperty::TowerHealthAdded,
		EUpgradeProperty::TowerHealthMultiplier,
	};

	for (EUpgradeProperty Property : StatProperties)
	{
		if (!Properties.Contains(Property))
		{
			Properties.Add(Property, 0.f);
		}
	}

	FTowerStats Stats;
	Stats.DamageAdded = FMath::RoundToInt(Properties.FindRef(EUpgradeProperty::TowerDamageAdded));
	Stats.DamageMultiplier = Properties.FindRef(EUpgradeProperty::TowerDamageMultiplier);
	Stats.CooldownReduction = Properties.FindRef(EUpgradeProperty::TowerCooldownReduction);
	Stats.CooldownMultiplier = Properties.FindRef(EUpgradeProperty::TowerCooldownMultiplier);
	Stats.RangeAdded = FMath::RoundToInt(Properties.FindRef(EUpgradeProperty::TowerRangeAdded));
	Stats.RangeMultiplier = Properties.FindRef(EUpgradeProperty::TowerRangeMultiplier);
	Stats.HealthAdded = FMath::RoundToInt(Properties.FindRef(EUpgradeProperty::TowerHealthAdded));
	Stats.HealthMultiplier = Properties.FindRef(EUpgradeProperty::TowerHealthMultiplier);

	for (EUpgradeProperty Property : StatProperties)
	{
		if (Properties.Contains(Property))
		{
			Properties.FindAndRemoveChecked(Property);
		}
	}
	
	Stats.SpecialProperties = Properties;

	TowerDataMap[TowerID].Stats = TowerDataMap[TowerID].Stats + Stats;

	// This will allow special stats to take effect
	OnTowerUpgraded.Broadcast(TowerID);
}

bool UTowerManagerSubsystem::GetTowerBaseStats(const FName& TowerID, FTowerData& TowerData)
{
	if (!TowerDataMap.Contains(TowerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Tower ID %s not found in TowerDataMap. This should not happen! Make sure to add the tower stats to the data table."), *TowerID.ToString());
		return false;
	}

	TowerData = TowerDataMap[TowerID];
	return true;
}
