// Fill out your copyright notice in the Description page of Project Settings.


#include "GlobalTowerSettings.h"
#include "EnemyManagerSubsystem.h"


void UEnemyManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadEnemyDataFromDataTable();
}

void UEnemyManagerSubsystem::LoadEnemyDataFromDataTable()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UDataTable* Table = Settings->EnemyDataTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load Enemy Data Table (Make sure to set it in Project Settings)"));
		return;
	}

	const TMap<FName, uint8*>& RowMap = Table->GetRowMap();

	for (auto& Pair : RowMap)
	{
		FName RowName = Pair.Key;

		FEnemyData* Data = reinterpret_cast<FEnemyData*>(Pair.Value);

		if (Data)
		{
			EnemyDataMap.Add(RowName, *Data);
		}
	}
}

void UEnemyManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

}

FEnemyData UEnemyManagerSubsystem::GetEnemyData(const FName& EnemyID)
{
	return EnemyDataMap[EnemyID];
}
