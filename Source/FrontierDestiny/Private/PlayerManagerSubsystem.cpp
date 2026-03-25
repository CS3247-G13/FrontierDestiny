// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalTowerSettings.h"
#include "UpgradeManagerSubsystem.h"
#include "PlayerManagerSubsystem.h"

void UPlayerManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UUpgradeManagerSubsystem>();
	LoadPlayerDataFromDataTable();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UUpgradeManagerSubsystem* UpgradeManager = GI->GetSubsystem<UUpgradeManagerSubsystem>())
		{
			UpgradeManager->OnUpgradePerformed.AddDynamic(this, &UPlayerManagerSubsystem::HandleUpgradePerformed);
			UpgradeManager->OnUpgradeActivated.AddDynamic(this, &UPlayerManagerSubsystem::HandleUpgradeActivated);
			UpgradeManager->OnUpgradeDeactivated.AddDynamic(this, &UPlayerManagerSubsystem::HandleUpgradeDeactivated);
		}
	}
}

void UPlayerManagerSubsystem::LoadPlayerDataFromDataTable()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UDataTable* Table = Settings->PlayerDataTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load Player Data Table."));
		return;
	}

	TArray<FName> RowNames = Table->GetRowNames();

	if (RowNames.Num() > 0)
	{
		FPlayerData* Data = Table->FindRow<FPlayerData>(RowNames[0], TEXT("PlayerManagerContext"));

		if (Data)
		{
			PlayerData = *Data;
			UE_LOG(LogTemp, Log, TEXT("PlayerManager: Successfully loaded stats from row: %s"), *RowNames[0].ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Player Data Table is empty!"));
	}
}

void UPlayerManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPlayerManagerSubsystem::SetUpgradeBoolean(const FName& TargetID, bool bValue)
{
	// Rifle
	if      (TargetID == "PiercingShots")    { PlayerData.bPiercingShots      = bValue; }
	else if (TargetID == "RuptureRounds")    { PlayerData.bRuptureRounds      = bValue; }
	else if (TargetID == "SuppressingFire")  { PlayerData.bSuppressingFire    = bValue; }
	else if (TargetID == "CompoundingInjury"){ PlayerData.bCompoundingInjury  = bValue; }
	else if (TargetID == "ArcShots")         { PlayerData.bArcShots           = bValue; }
	else if (TargetID == "ConduitMarker")    { PlayerData.bConduitMarker      = bValue; }
	// Shotgun
	else if (TargetID == "InfernoCartridge") { PlayerData.bInfernoCartridge   = bValue; }
	else if (TargetID == "FlakBarrel")       { PlayerData.bFlakBarrel         = bValue; }
	else if (TargetID == "StaggerShells")    { PlayerData.bStaggerShells      = bValue; }
	else if (TargetID == "BallisticRecall")  { PlayerData.bBallisticRecall    = bValue; }
	else if (TargetID == "SlugConversion")   { PlayerData.bSlugConversion     = bValue; }
	else if (TargetID == "DevastatingBlow")  { PlayerData.bDevastatingBlow    = bValue; }
	// Ammo
	else if (TargetID == "BulletReservoir1") { PlayerData.bBulletReservoir1   = bValue; }
	else if (TargetID == "BulletReservoir2") { PlayerData.bBulletReservoir2   = bValue; }
	else if (TargetID == "BulletReservoir3") { PlayerData.bBulletReservoir3   = bValue; }
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerManagerSubsystem: Unhandled upgrade TargetID '%s'. Make sure the DataTable row has a matching TargetID."), *TargetID.ToString());
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("Target: %s"), *TargetID.ToString());

	OnPlayerStatsChanged.Broadcast();
}

void UPlayerManagerSubsystem::HandleUpgradePerformed(const FUpgradeData& Upgrade)
{
	SetUpgradeBoolean(Upgrade.TargetID, true);
}

void UPlayerManagerSubsystem::HandleUpgradeActivated(const FUpgradeData& Upgrade)
{
	SetUpgradeBoolean(Upgrade.TargetID, true);
}

void UPlayerManagerSubsystem::HandleUpgradeDeactivated(const FUpgradeData& Upgrade)
{
	SetUpgradeBoolean(Upgrade.TargetID, false);
}
