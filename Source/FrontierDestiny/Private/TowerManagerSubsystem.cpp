// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerActor.h"
#include "GlobalTowerSettings.h"

#include "TowerManagerSubsystem.h"

void UTowerManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadTowerDataFromDataTable();
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
		FTowerData* Data = reinterpret_cast<FTowerData*>(Pair.Value);

		if (Data)
		{
			TowerDataMap.Add(Data->TowerID, *Data);
		}
	}
}

void UTowerManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

}

bool UTowerManagerSubsystem::GetTowerData(const FName& TowerID, FTowerData& TowerData)
{
	if (!TowerDataMap.Contains(TowerID))
	{
		return false;
	}

	TowerData = TowerDataMap[TowerID];
	return true;
}
