// Fill out your copyright notice in the Description page of Project Settings.

#include "ReconManagerSubsystem.h"
#include "CacheManagerSubsystem.h"
#include "EnemyManagerSubsystem.h"
#include "UpgradeManagerSubsystem.h"
#include "GlobalTowerSettings.h"
#include "Engine/World.h"

void UReconManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UUpgradeManagerSubsystem>();
	LoadBlipDataFromDataTable();

	if (UUpgradeManagerSubsystem* UpgradeManager = GetGameInstance()->GetSubsystem<UUpgradeManagerSubsystem>())
	{
		UpgradeManager->OnUpgradePerformed.AddDynamic(this, &UReconManagerSubsystem::HandleUpgradePerformed);
		UpgradeManager->OnUpgradeActivated.AddDynamic(this, &UReconManagerSubsystem::HandleUpgradeActivated);
		UpgradeManager->OnUpgradeDeactivated.AddDynamic(this, &UReconManagerSubsystem::HandleUpgradeDeactivated);
	}
}

void UReconManagerSubsystem::LoadBlipDataFromDataTable()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UDataTable* Table = Settings->BlipDataTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReconManagerSubsystem: Failed to load Blip Data Table (set it in Project Settings > Global Tower Settings)"));
		return;
	}

	const TMap<FName, uint8*>& RowMap = Table->GetRowMap();
	for (auto& Pair : RowMap)
	{
		FBlipData* Data = reinterpret_cast<FBlipData*>(Pair.Value);
		if (Data && Data->ID != NAME_None)
		{
			ResolvedSprites.Add(Data->ID, Data->Sprite.LoadSynchronous());
			ResolvedBlipSizes.Add(Data->ID, Data->BlipSize);
			ResolvedZOrders.Add(Data->ID, Data->ZOrder);
		}
	}
}

void UReconManagerSubsystem::SetReconBoolean(const FName& TargetID, bool bValue)
{
	if      (TargetID == "PriorityTargeting")   { bHasPriorityTargeting = bValue; }
	else if (TargetID == "CacheRadar")          { bCacheRadar           = bValue; }
	else if (TargetID == "CacheResonance")      { bCacheResonance       = bValue; }
	else if (TargetID == "HordeSurveillance")   { bHordeSurveillance    = bValue; }
	else { return; }

	UE_LOG(LogTemp, Display, TEXT("ReconManager: %s -> %s"), *TargetID.ToString(), bValue ? TEXT("enabled") : TEXT("disabled"));
}

void UReconManagerSubsystem::HandleUpgradePerformed(const FUpgradeData& Upgrade)
{
	UE_LOG(LogTemp, Display, TEXT("ReconManager: Upgrade purchased — TargetID '%s'"), *Upgrade.TargetID.ToString());
	SetReconBoolean(Upgrade.TargetID, true);
}

void UReconManagerSubsystem::HandleUpgradeActivated(const FUpgradeData& Upgrade)
{
	SetReconBoolean(Upgrade.TargetID, true);
}

void UReconManagerSubsystem::HandleUpgradeDeactivated(const FUpgradeData& Upgrade)
{
	SetReconBoolean(Upgrade.TargetID, false);
}

void UReconManagerSubsystem::GetVisibleBlips(FVector PlayerPos, float ViewRadius, float EnemyBlipRadius, TArray<FBlipRenderEntry>& OutBlips) const
{
	OutBlips.Reset();

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	UEnemyManagerSubsystem* EnemyManager = GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return;

	TArray<FMassEntityHandle> Handles;
	EnemyManager->GetEntitiesInRange(PlayerPos, EnemyBlipRadius, Handles);

	const float Diameter = ViewRadius * 2.0f;

	TObjectPtr<UMaterialInterface> const* EnemySprite         = ResolvedSprites.Find("enemy");
	TObjectPtr<UMaterialInterface> const* EnemyEnhancedSprite = ResolvedSprites.Find("enemy+");

	for (const FMassEntityHandle& Handle : Handles)
	{
		const FVector EnemyPos = EnemyManager->GetEntityPosition(Handle);
		const bool bEnhanced = bHasPriorityTargeting && EnemyManager->IsEnhancedEnemy(Handle);
		const FName BlipID = bEnhanced ? FName("enemy+") : FName("enemy");
		TObjectPtr<UMaterialInterface> const* Sprite = bEnhanced ? EnemyEnhancedSprite : EnemySprite;

		FVector2D Offset = FVector2D(EnemyPos.X - PlayerPos.X, EnemyPos.Y - PlayerPos.Y);
		if (Offset.Length() > ViewRadius)
		{
			Offset = Offset / Offset.Length() * ViewRadius;
		}
		FVector2D UV = FVector2D(
			Offset.Y / Diameter + 0.5f,
			1.0f - (Offset.X / Diameter + 0.5f)
		);

		FBlipRenderEntry Entry;
		Entry.MapUV = UV;
		Entry.Sprite = Sprite ? *Sprite : nullptr;
		if (const float* Size = ResolvedBlipSizes.Find(BlipID))
			Entry.BlipSize = *Size;
		if (const int32* Order = ResolvedZOrders.Find(BlipID))
			Entry.ZOrder = *Order;
		OutBlips.Add(Entry);
	}

	if (bCacheRadar)
	{
		UCacheManagerSubsystem* CacheManager = GetGameInstance()->GetSubsystem<UCacheManagerSubsystem>();
		if (CacheManager)
		{
			TObjectPtr<UMaterialInterface> const* CacheSprite = ResolvedSprites.Find("cache");

			for (const FVector& CachePos : CacheManager->GetCachesInRange(PlayerPos, ViewRadius))
			{
				FBlipRenderEntry Entry;
				Entry.MapUV = FVector2D(
					(CachePos.Y - PlayerPos.Y) / Diameter + 0.5f,
					1.0f - ((CachePos.X - PlayerPos.X) / Diameter + 0.5f)
				);
				Entry.Sprite = CacheSprite ? *CacheSprite : nullptr;
				if (const float* Size = ResolvedBlipSizes.Find("cache"))
					Entry.BlipSize = *Size;
				if (const int32* Order = ResolvedZOrders.Find("cache"))
					Entry.ZOrder = *Order;
				OutBlips.Add(Entry);
			}
		}
	}
}

UMaterialInterface* UReconManagerSubsystem::GetSpriteForID(FName ID) const
{
	TObjectPtr<UMaterialInterface> const* Sprite = ResolvedSprites.Find(ID);
	return Sprite ? *Sprite : nullptr;
}

float UReconManagerSubsystem::GetBlipSizeForID(FName ID) const
{
	const float* Size = ResolvedBlipSizes.Find(ID);
	return Size ? *Size : 24.0f;
}

int32 UReconManagerSubsystem::GetBlipZOrderForID(FName ID) const
{
	const int32* Order = ResolvedZOrders.Find(ID);
	return Order ? *Order : 0;
}
