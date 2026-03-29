// Fill out your copyright notice in the Description page of Project Settings.
#include "UpgradeManagerSubsystem.h"
#include "EconomySubsystem.h"
#include "GlobalTowerSettings.h"

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
		FUpgradeDataRow* Data = reinterpret_cast<FUpgradeDataRow*>(Pair.Value);
		if (Data)
		{
			if (Data->ID.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("UpgradeManager: Row '%s' has no ID set — skipping."), *Pair.Key.ToString());
				continue;
			}
			UpgradeMap.Add(Data->ID, FUpgradeData(*Data));
		}
	}

	for (auto& Pair : UpgradeMap)
	{
		const FName& UpgradeID = Pair.Key;
		FUpgradeData& Upgrade = Pair.Value;

		// Derive Unlocks from Prerequisites
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

		// Mirror mutual exclusions — if A lists B, ensure B also lists A.
		for (const FName& ExclusiveID : Upgrade.MutuallyExclusiveIDs)
		{
			if (UpgradeMap.Contains(ExclusiveID))
			{
				UpgradeMap[ExclusiveID].AddMutualExclusion(UpgradeID);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Mutually exclusive Upgrade ID %s not found for upgrade %s. Make sure to add all exclusive upgrades to the data table."), *ExclusiveID.ToString(), *UpgradeID.ToString());
			}
		}
	}

	// Initialise BlueprintMap — only for upgrades that require a blueprint, all starting as not collected.
	for (const auto& Pair : UpgradeMap)
	{
		if (Pair.Value.bRequiresBlueprint)
		{
			BlueprintMap.Add(Pair.Value.ID, false);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Initialized %d upgrades (%d require a blueprint)."), UpgradeMap.Num(), BlueprintMap.Num());
}

bool UUpgradeManagerSubsystem::PurchaseUpgrade(const FName& UpgradeID)
{
	if (!UpgradeMap.Contains(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade ID %s not found! Make sure to add it to the data table."), *UpgradeID.ToString());
		return false;
	}

	FUpgradeData& Upgrade = UpgradeMap[UpgradeID];

	// Check if upgrade is available (unlocked, not completed, not disabled)
	if (!Upgrade.IsAvailable())
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade ID %s is not available (locked, already completed, or disabled by a mutual exclusion)."), *UpgradeID.ToString());
		return false;
	}

	if (!CheckUpgradeSufficientResources(UpgradeID).bCanBePurchased)
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade ID %s could not be purchased due to insufficient resources."), *UpgradeID.ToString());
		return false;
	}

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

	Upgrade.CompleteUpgrade();

	// IF THIS IS NOT ALREADY DEACTIVATED
	if (!Upgrade.bNotMutuallyExcluded)
	{
		return true;
	}

	// Deactivate all mutually exclusive branches — they can still be purchased but won't take effect.
	for (const FName& ExclusiveID : Upgrade.MutuallyExclusiveIDs)
	{
		if (UpgradeMap.Contains(ExclusiveID))
		{
			DeactivateTree(ExclusiveID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Mutually exclusive Upgrade ID %s not found for upgrade %s. This should not happen!"), *ExclusiveID.ToString(), *UpgradeID.ToString());
		}
	}


	OnUpgradePerformed.Broadcast(Upgrade);

	return true;
}

void UUpgradeManagerSubsystem::UnlockBlueprint(const FName& UpgradeID)
{
	if (!BlueprintMap.Contains(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("No upgrade with ID %s was found that requires a blueprint!"), *UpgradeID.ToString());
		return;
	}
	BlueprintMap[UpgradeID] = true;
	if (!UpgradeMap.Contains(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrade ID %s not found in UpgradeMap! This should not happen if the data table is set up correctly."), *UpgradeID.ToString());
		return;
	}
	OnBlueprintUnlocked.Broadcast(UpgradeMap[UpgradeID]);
}

FUpgradeStatus UUpgradeManagerSubsystem::CheckUpgradeSufficientResources(const FName& UpgradeID)
{
	FUpgradeStatus Status;
	FUpgradeData Upgrade;
	if (!GetUpgradeData(UpgradeID, Upgrade))
	{
		UE_LOG(LogTemp, Warning, TEXT("CheckResources: Upgrade ID %s not found"), *UpgradeID.ToString());
		Status.bCanBePurchased = false;
		return Status;
	}

	if (Upgrade.bRequiresBlueprint)
	{
		Status.bRequiresBlueprint = true;
		const bool* bHasIt = BlueprintMap.Find(UpgradeID);
		Status.bHasBlueprint = bHasIt && *bHasIt;
		Status.bMissingBlueprint = !Status.bHasBlueprint;
	}

	UEconomySubsystem* Economy = GetGameInstance()->GetSubsystem<UEconomySubsystem>();
	if (Economy)
	{
		const FResourceAmount CurrentFunds = Economy->GetCurrentFunds();
		Status.RequiredResources = Upgrade.Cost;
		Status.CurrentResources = CurrentFunds;
		Status.bInsufficientCryxite = CurrentFunds.Cryxite < Upgrade.Cost.Cryxite;
		Status.bInsufficientNullsteel = CurrentFunds.Nullsteel < Upgrade.Cost.Nullsteel;
		Status.bInsufficientGravstone = CurrentFunds.Gravstone < Upgrade.Cost.Gravstone;
	}

	Status.bCanBePurchased =
		!Status.bMissingBlueprint &&
		!Status.bInsufficientCryxite &&
		!Status.bInsufficientNullsteel &&
		!Status.bInsufficientGravstone;

	return Status;
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

bool UUpgradeManagerSubsystem::ActivateUpgrade(const FName& UpgradeID)
{
	if (!UpgradeMap.Contains(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateUpgrade: Upgrade ID %s not found."), *UpgradeID.ToString());
		return false;
	}

	if (!IsInMutexChain(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateUpgrade: Upgrade ID %s is not part of a mutual exclusion chain."), *UpgradeID.ToString());
		return false;
	}

	const FName MutexRootID = FindMutexRoot(UpgradeID);
	if (MutexRootID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateUpgrade: Could not find mutex root for upgrade %s."), *UpgradeID.ToString());
		return false;
	}

	// Activate the root and all completed upgrades below it
	ActivateTree(MutexRootID);

	// Deactivate all sibling branches
	for (const FName& ExclusiveID : UpgradeMap[MutexRootID].MutuallyExclusiveIDs)
	{
		DeactivateTree(ExclusiveID);
	}

	return true;
}

bool UUpgradeManagerSubsystem::DeactivateUpgrade(const FName& UpgradeID)
{
	if (!UpgradeMap.Contains(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("DeactivateUpgrade: Upgrade ID %s not found."), *UpgradeID.ToString());
		return false;
	}

	if (!IsInMutexChain(UpgradeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("DeactivateUpgrade: Upgrade ID %s is not part of a mutual exclusion chain."), *UpgradeID.ToString());
		return false;
	}

	DeactivateTree(UpgradeID);
	return true;
}

bool UUpgradeManagerSubsystem::CanBePurchased(const FName& UpgradeID)
{
	FUpgradeData Upgrade;

	if (!GetUpgradeData(UpgradeID, Upgrade))
	{
		UE_LOG(LogTemp, Warning, TEXT("UpgradeManagerSubsystem: Upgrade ID %s not found."), *UpgradeID.ToString());
		return false;
	}

	return Upgrade.IsAvailable();
}

// --- Private helpers ---

bool UUpgradeManagerSubsystem::IsInMutexChain(const FName& UpgradeID) const
{
	if (!UpgradeMap.Contains(UpgradeID)) return false;

	const FUpgradeData& Upgrade = UpgradeMap[UpgradeID];
	if (Upgrade.MutuallyExclusiveIDs.Num() > 0) return true;

	for (const FName& PrereqID : Upgrade.Prerequisites)
	{
		if (IsInMutexChain(PrereqID)) return true;
	}

	return false;
}

FName UUpgradeManagerSubsystem::FindMutexRoot(const FName& UpgradeID) const
{
	if (!UpgradeMap.Contains(UpgradeID)) return NAME_None;

	const FUpgradeData& Upgrade = UpgradeMap[UpgradeID];
	if (Upgrade.MutuallyExclusiveIDs.Num() > 0) return UpgradeID;

	for (const FName& PrereqID : Upgrade.Prerequisites)
	{
		const FName Root = FindMutexRoot(PrereqID);
		if (!Root.IsNone()) return Root;
	}

	return NAME_None;
}

void UUpgradeManagerSubsystem::DeactivateTree(const FName& UpgradeID)
{
	if (!UpgradeMap.Contains(UpgradeID)) return;

	FUpgradeData& Upgrade = UpgradeMap[UpgradeID];
	if (!Upgrade.bNotMutuallyExcluded) return; // Already deactivated — avoids redundant broadcasts

	Upgrade.Deactivate();
	OnUpgradeDeactivated.Broadcast(Upgrade);

	for (const FName& UnlockID : Upgrade.Unlocks)
	{
		DeactivateTree(UnlockID);
	}
}

void UUpgradeManagerSubsystem::ActivateTree(const FName& UpgradeID)
{
	if (!UpgradeMap.Contains(UpgradeID)) return;

	FUpgradeData& Upgrade = UpgradeMap[UpgradeID];
	if (Upgrade.bNotMutuallyExcluded) return; // Already active — avoids redundant broadcasts


	Upgrade.Activate();

	for (const FName& UnlockID : Upgrade.Unlocks)
	{
		ActivateTree(UnlockID);
	}

	// Only call activate if the player actually purchased this upgrade
	if (!Upgrade.bIsPurchased) return;

	OnUpgradeActivated.Broadcast(Upgrade);
}
