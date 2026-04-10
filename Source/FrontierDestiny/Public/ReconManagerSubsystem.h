// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BlipData.h"
#include "Upgrade.h"
#include "ReconManagerSubsystem.generated.h"

UCLASS()
class FRONTIERDESTINY_API UReconManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Recon upgrades
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recon")
	bool bHasPriorityTargeting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recon")
	bool bCacheRadar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recon")
	bool bCacheResonance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recon")
	bool bHordeSurveillance = false;

	/**
	 * Returns one render entry per visible entity.
	 * Enemies within ViewRadius are placed normally; enemies within EnemyBlipRadius (but outside ViewRadius)
	 * are included with bClamped=true — the widget clamps them to the minimap edge.
	 * MapUV on each entry is in minimap-relative UV space: (0.5, 0.5) = player, (0,0)/(1,1) = view radius edges.
	 */
	void GetVisibleBlips(FVector PlayerPos, float ViewRadius, float EnemyBlipRadius, TArray<FBlipRenderEntry>& OutBlips) const;

	UFUNCTION(BlueprintCallable, Category = "Recon")
	UMaterialInterface* GetSpriteForID(FName ID) const;

	UFUNCTION(BlueprintCallable, Category = "Recon")
	float GetBlipSizeForID(FName ID) const;

	UFUNCTION(BlueprintCallable, Category = "Recon")
	int32 GetBlipZOrderForID(FName ID) const;

	UFUNCTION()
	void HandleUpgradePerformed(const FUpgradeData& Upgrade);

	UFUNCTION()
	void HandleUpgradeActivated(const FUpgradeData& Upgrade);

	UFUNCTION()
	void HandleUpgradeDeactivated(const FUpgradeData& Upgrade);

private:
	void LoadBlipDataFromDataTable();
	void SetReconBoolean(const FName& TargetID, bool bValue);

	UPROPERTY()
	TMap<FName, TObjectPtr<UMaterialInterface>> ResolvedSprites;
	UPROPERTY()
	TMap<FName, float> ResolvedBlipSizes;
	UPROPERTY()
	TMap<FName, int32> ResolvedZOrders;
};
