// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Upgrade.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UpgradeManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrerequisiteMet, FName, PrerequisiteID, FName, UnlockID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradePerformed, const FUpgradeData&, Upgrade);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUpgradeDisabled, FName, PerformedUpgradeID, FName, DisabledUpgradeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeActivated, const FUpgradeData&, Upgrade);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeDeactivated, const FUpgradeData&, Upgrade);

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FUpgradeStatus
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bRequiresBlueprint = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasBlueprint = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FResourceAmount RequiredResources;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FResourceAmount CurrentResources;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMissingBlueprint = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bInsufficientCryxite = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bInsufficientNullsteel = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bInsufficientGravstone = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCanBePurchased = false;
};

UCLASS()
class FRONTIERDESTINY_API UUpgradeManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void LoadUpgradesFromDataTable();

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool PurchaseUpgrade(const FName& UpgradeID);

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	FUpgradeStatus CheckUpgradeSufficientResources(const FName& UpgradeID);

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool GetUpgradeData(const FName& UpgradeID, FUpgradeData& Upgrade) const;

	// Activates the mutex branch this upgrade belongs to and deactivates all sibling branches.
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool ActivateUpgrade(const FName& UpgradeID);

	// Deactivates this upgrade and all completed upgrades below it in the unlock tree.
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool DeactivateUpgrade(const FName& UpgradeID);

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool CanBePurchased(const FName& UpgradeID);

	TMap<FName, FUpgradeData> UpgradeMap;

	// Maps upgrade ID to whether the player has collected its blueprint.
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	TMap<FName, bool> BlueprintMap;

	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnPrerequisiteMet OnPrerequisiteMet;
	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnUpgradePerformed OnUpgradePerformed;
	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnUpgradeDisabled OnUpgradeDisabled;
	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnUpgradeActivated OnUpgradeActivated;
	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnUpgradeDeactivated OnUpgradeDeactivated;

private:
	// Returns true if the upgrade itself or any ancestor has mutual exclusions.
	bool IsInMutexChain(const FName& UpgradeID) const;

	// Walks up the prerequisite chain to find the ancestor with mutual exclusions.
	FName FindMutexRoot(const FName& UpgradeID) const;

	// Recursively deactivates an upgrade and all completed unlocks below it.
	void DeactivateTree(const FName& UpgradeID);

	// Recursively activates all *completed* upgrades starting from UpgradeID downward.
	void ActivateTree(const FName& UpgradeID);
};
