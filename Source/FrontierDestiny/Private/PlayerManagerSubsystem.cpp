// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalTowerSettings.h"
#include "UpgradeSubsystem.h"
#include "PlayerManagerSubsystem.h"

void UPlayerManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
    Collection.InitializeDependency<UUpgradeManagerSubsystem>();
	LoadPlayerDataFromDataTable();
    
    // Subscribe to the Upgrade Subsystem
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUpgradeManagerSubsystem* UpgradeManager = GI->GetSubsystem<UUpgradeManagerSubsystem>())
        {
            UpgradeManager->OnUpgradePerformed.AddDynamic(this, &UPlayerManagerSubsystem::HandleUpgradePerformed);
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

    // Since there is only one row, we can just get all row names and take the first one
    TArray<FName> RowNames = Table->GetRowNames();

    if (RowNames.Num() > 0)
    {
        // FindRow is safer than reinterpret_cast for DataTables
        FPlayerData* Data = Table->FindRow<FPlayerData>(RowNames[0], TEXT("PlayerManagerContext"));

        if (Data)
        {
            // Store the data into a single member variable instead of a Map
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

void UPlayerManagerSubsystem::HandleUpgradePerformed(const FUpgradeData& Upgrade)
{
	if (Upgrade.TargetID == "rifle" || Upgrade.TargetID == "shotgun")
	{
		// Accessing the Map inside PlayerData (or PlayerManagerSubsystem)
		FWeaponData& Data = PlayerData.WeaponDataMap[
			Upgrade.TargetID == "rifle"
				? EWeaponType::Rifle
				: Upgrade.TargetID == "shotgun"
				? EWeaponType::Shotgun
				: EWeaponType::Rifle];

		static const TArray<EUpgradeProperty> WeaponStatProperties = {
			EUpgradeProperty::WeaponDamageAdded,
			EUpgradeProperty::WeaponDamageMultiplier,
			EUpgradeProperty::WeaponFireRateAdded,
			EUpgradeProperty::WeaponFireRateMultiplier,
			EUpgradeProperty::WeaponSpreadReduction,
		};

		// Copy it so we can manipulate it to add defaults
		TMap<EUpgradeProperty, float> Properties = Upgrade.Properties;
		for (EUpgradeProperty Property : WeaponStatProperties)
		{
			if (!Properties.Contains(Property))
			{
				Properties.Add(Property, 0.f);
			}
		}

		// Apply the stat changes to the persistent data
		Data.DamageAdded += Properties[EUpgradeProperty::WeaponDamageAdded];
		Data.DamageMultiplier += Properties[EUpgradeProperty::WeaponDamageMultiplier];
		Data.FireRateAdded += Properties[EUpgradeProperty::WeaponFireRateAdded];
		Data.FireRateMultiplier += Properties[EUpgradeProperty::WeaponFireRateMultiplier];
		Data.SpreadReduction += Properties[EUpgradeProperty::WeaponSpreadReduction];

		// Broadcast so CombatComponent can react to weapon changes
		OnPlayerStatsChanged.Broadcast();
		return;
	}

	else if (Upgrade.TargetID == "player")
	{
		// Handle the player upgrade
		static const TArray<EUpgradeProperty> PlayerStatProperties = {
			EUpgradeProperty::PlayerAmmoReplenishRateAdded,
			EUpgradeProperty::PlayerAmmoReplenishRateMultiplier
		};
		TMap<EUpgradeProperty, float> Properties = Upgrade.Properties;
		for (EUpgradeProperty Property : PlayerStatProperties)
		{
			if (!Properties.Contains(Property))
			{
				Properties.Add(Property, 0.f);
			}
		}

		// Applying to PlayerData members
		PlayerData.AmmoReplenishRateAdded += Properties[EUpgradeProperty::PlayerAmmoReplenishRateAdded];
		PlayerData.AmmoReplenishRateMultiplier += Properties[EUpgradeProperty::PlayerAmmoReplenishRateMultiplier];

		// Broadcast so CombatComponent calls UpdateBulletReplenishTimer()
		OnPlayerStatsChanged.Broadcast();
	}
}