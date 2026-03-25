// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ResourceAmount.h"
#include "Upgrade.generated.h"

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FUpgradeDataRow : public FTableRowBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FName ID = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Name = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Description = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FName TargetID = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FResourceAmount Cost;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	bool bRequiresBlueprint = false;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Special")
	TMap<FGameplayTag, float> Properties;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> PrerequisiteUpgradeIDs;
	// Only needs to be filled on one side — the subsystem mirrors it to the other at load time.
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Exclusion")
	TSet<FName> MutuallyExclusiveIDs;
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FUpgradeData
{
	GENERATED_BODY()
public:
	FUpgradeData()
	{}

	FUpgradeData(const FUpgradeDataRow& Data):
		ID(Data.ID),
		Name(Data.Name),
		Description(Data.Description),
		TargetID(Data.TargetID),
		Icon(Data.Icon),
		Cost(Data.Cost),
		bRequiresBlueprint(Data.bRequiresBlueprint),
		Properties(Data.Properties),
		Prerequisites(Data.PrerequisiteUpgradeIDs),
		RemainingPrerequisites(Data.PrerequisiteUpgradeIDs),
		MutuallyExclusiveIDs(Data.MutuallyExclusiveIDs)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FName ID = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Name = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FString Description = "";
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FName TargetID = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FResourceAmount Cost;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	bool bRequiresBlueprint = false;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Special")
	TMap<FGameplayTag, float> Properties;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> Prerequisites;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> Unlocks;
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Prerequisites")
	TSet<FName> RemainingPrerequisites;

	// Fully mirrored at load time — if A excludes B, B will also list A.
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade|Exclusion")
	TSet<FName> MutuallyExclusiveIDs;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	bool bIsPurchased = false;
	// Whether this upgrade's effects are currently applied. False when a sibling mutex branch is active.
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	bool bNotMutuallyExcluded = true;

	void AddUnlock(FName UpgradeID)
	{
		Unlocks.Add(UpgradeID);
	}

	void AddMutualExclusion(FName UpgradeID)
	{
		MutuallyExclusiveIDs.Add(UpgradeID);
	}

	void ClearPrerequisite(FName UpgradeID)
	{
		RemainingPrerequisites.Remove(UpgradeID);
	}

	void CompleteUpgrade()
	{
		bIsPurchased = true;
	}

	void Activate()
	{
		bNotMutuallyExcluded = true;
	}

	void Deactivate()
	{
		bNotMutuallyExcluded = false;
	}

	bool IsUnlocked() const
	{
		return RemainingPrerequisites.IsEmpty();
	}

	bool IsAvailable() const
	{
		return IsUnlocked() && !bIsPurchased;
	}
};
