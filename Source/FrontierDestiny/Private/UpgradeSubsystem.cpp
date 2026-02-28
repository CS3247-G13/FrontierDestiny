// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerManagerSubsystem.h"
#include "GlobalTowerSettings.h"
#include "UpgradeSubsystem.h"

void UUpgradeManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadUpgradesFromDataTable();
}

void UUpgradeManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UUpgradeManagerSubsystem::LoadUpgradesFromDataTable()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UDataTable* Table = Settings->UpgradeDataTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load Upgrade Data Table (Make sure to set it in Project Settings)."));
		return;
	}

	const TMap<FName, uint8*>& RowMap = Table->GetRowMap();
	for (auto& Pair : RowMap)
	{
		FName RowName = Pair.Key;

		FUpgradeDataRow* Data = reinterpret_cast<FUpgradeDataRow*>(Pair.Value);

		if (Data)
		{
			FUpgradeData Upgrade = FUpgradeData(*Data);
			UpgradeMap.Add(RowName, Upgrade);
		}
	}

	for (auto& Pair : UpgradeMap)
	{
		const FName& UpgradeID = Pair.Key;
		FUpgradeData& Upgrade = Pair.Value;

		for (const FName& PrerequisiteID : Upgrade.Prerequisites)
		{
			if (UpgradeMap.Contains(PrerequisiteID))
			{
				UpgradeMap[PrerequisiteID].AddUnlock(UpgradeID);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Prerequisite Upgrade ID %s not found for upgrade %s. This should not happen! Make sure to add all prerequisite upgrades to the data table."), *PrerequisiteID.ToString(), *UpgradeID.ToString());
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Initialized %d tower upgrades."), UpgradeMap.Num());
}

bool UUpgradeManagerSubsystem::PerformUpgrade(const FName& UpgradeID)
{
	// Check if upgrade exists
	if (!UpgradeMap.Contains(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade ID %s not found! Make sure to add it to the data table."), *UpgradeID.ToString());
		return false;
	}
	FUpgradeData& Upgrade = UpgradeMap[UpgradeID];
	// Check if upgrade is upgradable
	if (!Upgrade.IsUnlocked())
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade ID %s not unlocked!"), *UpgradeID.ToString());
		return false;
	}

	// TODO: Check for resources to upgrade

	// Clear the things that the upgrade unlocks as prerequisites, and broadcast that their unlock state is updated
	for (auto& UnlockID : Upgrade.Unlocks)
	{
		if (UpgradeMap.Contains(UnlockID))
		{
			UpgradeMap[UnlockID].ClearPrerequisite(UpgradeID);
			OnPrerequisiteMet.Broadcast(UpgradeID, UnlockID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unlock Upgrade ID %s not found for upgrade %s. This should not happen! Make sure to add all unlock upgrades to the data table."), *UnlockID.ToString(), *UpgradeID.ToString());
		}
	}

	// Set the upgrade as completed
	Upgrade.CompleteUpgrade();

	// Broadcast the upgrade performed event
	OnUpgradePerformed.Broadcast(Upgrade);

	return true;
}

bool UUpgradeManagerSubsystem::GetUpgradeData(const FName& UpgradeID, FUpgradeData& Upgrade) const
{
	if (UpgradeMap.Contains(UpgradeID))
	{
		Upgrade = UpgradeMap[UpgradeID];
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade ID %s not found! Make sure to add it to the data table."), *UpgradeID.ToString());
		return false;
	}
}
