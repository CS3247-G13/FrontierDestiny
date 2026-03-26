// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemySpawner.h"
#include "CoreManagerSubsystem.h"
#include "EnemyManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"

AEnemySpawner::AEnemySpawner()
{
	bAutoSpawnOnBeginPlay = false;
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (UEnemyWaveManagerSubsystem* WaveManager = GetWorld()->GetSubsystem<UEnemyWaveManagerSubsystem>())
	{
		WaveManager->OnSpawnOrderIssued.AddDynamic(this, &AEnemySpawner::HandleSpawnOrder);
		UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Subscribed to OnSpawnOrderIssued (Tag='%s', CoreIndex=%d, bIsBackup=%d)"),
			*GetName(), *Tag.ToString(), CoreIndex, bIsBackup);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: Could not find EnemyWaveManagerSubsystem — spawn orders will not be received."), *GetName());
	}
}

void AEnemySpawner::HandleSpawnOrder(FGameplayTag SpawnTag, const FHordeBatchDetails& Details)
{
	if (SpawnTag == Tag)
	{
		UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Received matching spawn order for Tag '%s'."), *GetName(), *SpawnTag.ToString());
		Spawn(Details);
	}
}

void AEnemySpawner::Spawn(const FHordeBatchDetails& Details)
{
	UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Spawn called — %d enemy entries."), *GetName(), Details.Enemies.Num());

	UCoreManagerSubsystem* CoreManager = GetWorld()->GetSubsystem<UCoreManagerSubsystem>();
	if (!CoreManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: CoreManagerSubsystem not found."), *GetName());
		return;
	}

	ACoreActor* Core = CoreManager->GetCore(CoreIndex);
	if (!Core)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: No CoreActor registered for CoreIndex %d."), *GetName(), CoreIndex);
		return;
	}
	if (!Core->bIsCoreActive)
	{
		UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Core %d is not active — skipping spawn."), *GetName(), CoreIndex);
		return;
	}

	if (!bIsBackup && BackupSpawnerTag.IsValid())
	{
		APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (Player && FVector::Dist(Player->GetActorLocation(), GetActorLocation()) < DisableDistance)
		{
			UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Player too close (%.0f < %.0f) — redirecting to backup '%s'."),
				*GetName(), FVector::Dist(Player->GetActorLocation(), GetActorLocation()), DisableDistance, *BackupSpawnerTag.ToString());
			if (UEnemyWaveManagerSubsystem* WaveManager = GetWorld()->GetSubsystem<UEnemyWaveManagerSubsystem>())
			{
				WaveManager->OnSpawnOrderIssued.Broadcast(BackupSpawnerTag, Details);
			}
			return;
		}
	}

	UEnemyManagerSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: EnemyManagerSubsystem not found."), *GetName());
		return;
	}

	int32 TotalCount = 0;
	for (const auto& EnemyInfo : Details.Enemies)
	{
		TotalCount += EnemyInfo.Count;
	}

	if (TotalCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: TotalCount is 0 — nothing to spawn."), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Spawning %d enemies across %d types."), *GetName(), TotalCount, Details.Enemies.Num());

	EntityTypes.Empty();
	for (const auto& EnemyInfo : Details.Enemies)
	{
		FEnemyData EnemyData = Subsystem->GetEnemyData(EnemyInfo.EnemyID);
		if (!EnemyData.EnemyMassEntityAsset.IsNull())
		{
			UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]:   EnemyID '%s' x%d — asset '%s'"),
				*GetName(), *EnemyInfo.EnemyID.ToString(), EnemyInfo.Count, *EnemyData.EnemyMassEntityAsset.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]:   EnemyID '%s' has no MassEntityAsset set — will spawn nothing for this type."),
				*GetName(), *EnemyInfo.EnemyID.ToString());
		}
		FMassSpawnedEntityType Type;
		Type.EntityConfig = EnemyData.EnemyMassEntityAsset;
		Type.Proportion = (float)EnemyInfo.Count / (float)TotalCount;
		EntityTypes.Add(Type);
	}

	Count = TotalCount;
	UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Calling DoSpawning()."), *GetName());
	DoSpawning();
}
