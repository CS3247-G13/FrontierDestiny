// Fill out your copyright notice in the Description page of Project Settings.


#include "GlobalTowerSettings.h"
#include "Async/TaskGraphInterfaces.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "MassEntitySubsystem.h"

#include "MassRepresentationSubsystem.h"
#include "EnemyDamageMassProcessor.h"

#include "EnemyManagerSubsystem.h"

#include "QuestSubsystem.h"

void UEnemyManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadEnemyDataFromDataTable();
	InitializeHealthbars();
}

void UEnemyManagerSubsystem::InitializeHealthbars()
{
	HealthRatios.SetNum(1000);
	EnemyPositions.SetNum(1000);
	EnemyVisibilities.SetNum(1000);
	ActiveEntityHandles.Reserve(1000);
}

FMassEntityHandle UEnemyManagerSubsystem::GetEnemyEntityHandle(UInstancedStaticMeshComponent* Component, int32 Item) const
{
	FMassEntityHandle Handle;

	// Reverse map the ISMC to FMassEntityHandle
	UMassRepresentationSubsystem* RepSubsystem = GetWorld()->GetSubsystem<UMassRepresentationSubsystem>();

	const FMassISMCSharedData* SharedData = RepSubsystem->GetISMCSharedDataForInstancedStaticMesh(Component);

	if (SharedData)
	{
		for (const auto& Pair : SharedData->GetEntityPrimitiveToIdMap())
		{
			if (Component->GetInstanceIndexForId(Pair.Value) == Item)
			{
				Handle = Pair.Key;
			}
		}
	}

	return Handle;
}

void UEnemyManagerSubsystem::ApplyDamageToEnemy(FMassEntityHandle Handle, int DamageThisHit)
{
	// Handle exists
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();

	if (EntitySubsystem)
	{
		// 1. Access the EntityManager from the Subsystem
		const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();

		// 2. Use Defer() to get the system-managed Command Buffer
		FMassCommandBuffer& CommandBuffer = EntityManager.Defer();

		// 3. Add Damage instead
		CommandBuffer.PushCommand<FMassDeferredSetCommand>(
			[Handle, DamageThisHit](FMassEntityManager& Manager)
			{
				if (!Manager.IsEntityValid(Handle)) return;

				FDamageFragment* Damage = Manager.GetFragmentDataPtr<FDamageFragment>(Handle);
				if (Damage)
				{
					// Fragment exists — accumulate
					Damage->DamageAmount += DamageThisHit;
				}
				else
				{
					// Fragment missing — add it with initial value via initializer callback
					Manager.AddFragmentToEntity(Handle, FDamageFragment::StaticStruct(),
						[DamageThisHit](void* Fragment, const UScriptStruct&)
						{
							static_cast<FDamageFragment*>(Fragment)->DamageAmount = DamageThisHit;
						});
				}
			}
		);
	}
}

void UEnemyManagerSubsystem::DestroyEnemyByISMC(UInstancedStaticMeshComponent* Component, int32 Item)
{
	FMassEntityHandle Handle = GetEnemyEntityHandle(Component, Item);

	// Handle exists
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();

	if (EntitySubsystem)
	{
		// 1. Access the EntityManager from the Subsystem
		const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();

		// 2. Use Defer() to get the system-managed Command Buffer
		FMassCommandBuffer& CommandBuffer = EntityManager.Defer();

		// 3. Destroy Enemy
		CommandBuffer.DestroyEntity(Handle);
	}
}

void UEnemyManagerSubsystem::AssignNiagaraComponent(UNiagaraComponent* Component)
{
	NiagaraComponent = Component;
}

void UEnemyManagerSubsystem::UpdateHealthbarInformation(TArray<float>& UpdatedHealthRatios, TArray<FVector>& UpdatedEnemyPositions, TArray<FMassEntityHandle>& UpdatedEntityHandles)
{
	// Update stored arrays
	HealthRatios = UpdatedHealthRatios;
	EnemyPositions = UpdatedEnemyPositions;
	ActiveEntityHandles = UpdatedEntityHandles;

	// Set visibility — 1.0 for active entities, 0.0 for empty slots
	for (int32 i = 0; i < EnemyVisibilities.Num(); i++)
	{
		EnemyVisibilities[i] = i < EnemyPositions.Num() ? 1.0f : 0.0f;
	}

	if (!NiagaraComponent) return;

	// Convert FVector to FNiagaraPosition
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
		NiagaraComponent, FName("Enemy Positions"), EnemyPositions);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Health Ratios"), HealthRatios);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Visibilities"), EnemyVisibilities);
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

void UEnemyManagerSubsystem::NotifyEnemyDeath(FMassEntityHandle Handle)
{
	// Called from a Mass processor which may be off the game thread.
	// Defer to game thread since broadcast subscribers touch Blueprint/timer systems.
	AsyncTask(ENamedThreads::GameThread, [this, Handle]()
	{
		OnEnemyDeath.Broadcast(Handle);

		if (UQuestSubsystem* QuestSubsystem = GetGameInstance()->GetSubsystem<UQuestSubsystem>())
		{
			QuestSubsystem->RegisterEnemyKilled();
		}
	});
}

void UEnemyManagerSubsystem::ApplyDamageToTarget(FMassEnemyTarget Target, int32 Damage)
{
	ApplyDamageToEnemy(Target.EntityHandle, Damage);
}

bool UEnemyManagerSubsystem::IsTargetValid(FMassEnemyTarget Target) const
{
	return Target.EntityHandle.IsSet();
}

bool UEnemyManagerSubsystem::IsSameTarget(FMassEnemyTarget A, FMassEnemyTarget B) const
{
	return A.EntityHandle == B.EntityHandle;
}

bool UEnemyManagerSubsystem::GetEnemyTargetFromHit(const FHitResult& Hit, FMassEnemyTarget& OutTarget) const
{
	UInstancedStaticMeshComponent* ISMC = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	if (!ISMC || Hit.Item == INDEX_NONE)
	{
		return false;
	}

	FMassEntityHandle Handle = GetEnemyEntityHandle(ISMC, Hit.Item);
	if (!Handle.IsSet())
	{
		return false;
	}

	OutTarget.EntityHandle = Handle;
	OutTarget.Position = GetEntityPosition(Handle);
	return true;
}

void UEnemyManagerSubsystem::GetEntitiesInRange(FVector Center, float Radius, TArray<FMassEntityHandle>& OutHandles) const
{
	const float RadiusSq = Radius * Radius;
	for (int32 i = 0; i < ActiveEntityHandles.Num(); i++)
	{
		if (i < EnemyPositions.Num() && FVector::DistSquared(Center, EnemyPositions[i]) <= RadiusSq)
		{
			OutHandles.Add(ActiveEntityHandles[i]);
		}
	}
}

FVector UEnemyManagerSubsystem::GetEntityPosition(FMassEntityHandle Handle) const
{
	const int32 Idx = ActiveEntityHandles.IndexOfByKey(Handle);
	return (Idx != INDEX_NONE && Idx < EnemyPositions.Num()) ? EnemyPositions[Idx] + FVector(0.0f, 0.0f, 100.0f) : FVector::ZeroVector;
}

float UEnemyManagerSubsystem::GetEntityHealth(FMassEntityHandle Handle) const
{
	const int32 Idx = ActiveEntityHandles.IndexOfByKey(Handle);
	return (Idx != INDEX_NONE && Idx < HealthRatios.Num()) ? HealthRatios[Idx] : 0.f;
}

void UEnemyManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

FEnemyData UEnemyManagerSubsystem::GetEnemyData(const FName& EnemyID)
{
	return EnemyDataMap[EnemyID];
}
