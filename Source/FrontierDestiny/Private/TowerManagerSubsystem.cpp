// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerManagerSubsystem.h"
#include "TowerActor.h"
#include "GlobalTowerSettings.h"
#include "UpgradeManagerSubsystem.h"


void UTowerManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UUpgradeManagerSubsystem>();
	LoadTowerDataFromDataTable();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UUpgradeManagerSubsystem* UpgradeManager = GI->GetSubsystem<UUpgradeManagerSubsystem>())
		{
			UpgradeManager->OnUpgradePerformed.AddDynamic(this, &UTowerManagerSubsystem::HandleUpgradePerformed);
		}
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UTowerManagerSubsystem::OnLevelChanged
	);
}

void UTowerManagerSubsystem::OnLevelChanged(UWorld* World)
{
	UnlockedTowers.Empty();
}

// Checks if the tower at the very end of the path exists. Does not check along the way
bool UTowerManagerSubsystem::CheckPathUnlocked(TArray<int32> Path)
{
	FTowerPathNode Node = FullTowerPath;
	for (int32 Edge : Path)
	{
		TSharedPtr<FTowerPathNode>* Next = Node.Children.Find(Edge);
		if (Next == nullptr)
		{
			return false;
		}
		Node = **Next;
	}
	if (Node.TowerID == NAME_None)
	{
		return false;
	}
	return UnlockedTowers.Contains(Node.TowerID);
}

// Returns the next set of towers after the input path
TMap<int32, FTowerData> UTowerManagerSubsystem::GetPathNextTowers(TArray<int32> Path)
{
	FTowerPathNode Node = FullTowerPath;
	for (int32 Edge : Path)
	{
		TSharedPtr<FTowerPathNode>* Next = Node.Children.Find(Edge);
		if (Next == nullptr)
		{
			// Hmm there is no such path. Something went very wrong.
			UE_LOG(LogTemp, Warning, TEXT("GetPathNextTowers: No such path. Something went very wrong."));
			return TMap<int32, FTowerData>();
		}
		Node = **Next;
	}

	TMap<int32, TSharedPtr<FTowerPathNode>> Children = Node.Children;
	
	// Convert TSharedPtr<FTowerPathnode> to FTowerData
	TMap<int32, FTowerData> Result;
	for (const auto& Pair : Children)
	{
		FTowerPathNode CurrentNode = *Pair.Value;
		FTowerData* Data;
		if (CurrentNode.TowerID == NAME_None)
		{
			// This is the end of this path
			continue;
		}
		Data = TowerDataMap.Find(CurrentNode.TowerID);
		if (!Data)
		{
			// Something went wrong and the tower cannot be found
			UE_LOG(LogTemp, Warning, TEXT("GetPathNextTowers: Tower with ID %s not found!"), *CurrentNode.TowerID.ToString());
			continue;
		}
		if (!UnlockedTowers.Contains(CurrentNode.TowerID))
		{
			// Not unlocked yet
			continue;
		}

		Result.Add(Pair.Key, *Data);
	}
	return Result;
}

// Returns the tower at the end of the path
FTowerData UTowerManagerSubsystem::GetPathTower(TArray<int32> Path)
{
	FTowerPathNode Node = FullTowerPath;
	for (int32 Edge : Path)
	{
		TSharedPtr<FTowerPathNode>* Next = Node.Children.Find(Edge);
		if (Next == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("GetPathTower: No path found. Something went very wrong."));
			return FTowerData();
		}
		Node = **Next;
	}
	if (Node.TowerID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetPathTower: No tower found at the end of the path. Something went very wrong."));
		return FTowerData();
	}
	return TowerDataMap.FindRef(Node.TowerID);
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
			if (Data->ID.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("TowerManager: Row '%s' has no ID set — skipping."), *Pair.Key.ToString());
				continue;
			}
			TowerDataMap.Add(Data->ID, *Data);

			FTowerPathNode* Node = &FullTowerPath;
			for (int32 Edge : Data->SelectionPath)
			{
				if (!Node->Children.Contains(Edge))
				{
					Node->Children.Add(Edge, MakeShared<FTowerPathNode>());
				}
				Node = Node->Children.Find(Edge)->Get();
			}
			Node->TowerID = Data->ID;
		}
	}
}

void UTowerManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

}

bool UTowerManagerSubsystem::IsTowerUnlocked(FName TowerID) const
{
	return UnlockedTowers.Contains(TowerID);
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

void UTowerManagerSubsystem::HandleUpgradePerformed(const FUpgradeData& UpgradeData)
{
	if (!TowerDataMap.Contains(UpgradeData.TargetID) || UnlockedTowers.Contains(UpgradeData.TargetID))
	{
		// This was not a tower upgrade or it was already unlocked somehow
		return;
	}
	FTowerData* Data = TowerDataMap.Find(UpgradeData.TargetID);
	if (Data == nullptr)
	{
		return;
	}
	UnlockedTowers.Add(UpgradeData.TargetID);
	OnTowerUnlocked.Broadcast(*Data);
}
