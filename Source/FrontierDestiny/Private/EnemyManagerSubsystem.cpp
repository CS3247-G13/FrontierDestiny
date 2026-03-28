// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyManagerSubsystem.h"

#include "GlobalTowerSettings.h"
#include "Async/TaskGraphInterfaces.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "MassEntitySubsystem.h"

#include "MassRepresentationSubsystem.h"
#include "EnemyDamageMassProcessor.h"

#include "StatusEffectFragments.h"
#include "DamageNumber.h"

#include "QuestSubsystem.h"

static constexpr float MitigatedFadeDuration = 1.5f;

void UEnemyManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadEnemyDataFromDataTable();
	InitializeHealthbars();

	OnEnemyDamageTaken.AddUObject(this, &UEnemyManagerSubsystem::SpawnHitEffects);
}

void UEnemyManagerSubsystem::InitializeHealthbars()
{
	EnemyHealths.SetNum(1000);
	EnemyMaxHealths.SetNum(1000);
	EnemyPositions.SetNum(1000);
	EnemyVisibilities.SetNum(1000);
	ActiveEntityHandles.SetNum(1000);
	EnemyVitalities.SetNum(1000);
	BurnDurations.SetNum(1000);
	SlowDurations.SetNum(1000);
	StunDurations.SetNum(1000);
	ModifierFlags.SetNum(1000);
	FragmentedChunkSizes.SetNum(1000);
	MitigatedDamageAmounts.SetNum(1000);
	MitigatedDamageFadeTimers.SetNum(1000);
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

void UEnemyManagerSubsystem::SpawnHitEffects(FVector Location, int32 Damage)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();

	if (UNiagaraSystem* Splatter = Settings->BloodSplatterEffect.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Splatter, Location);
	}

	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ADamageNumber* Num = World->SpawnActor<ADamageNumber>(ADamageNumber::StaticClass(), Location, FRotator::ZeroRotator, Params);
		if (Num)
		{
			Num->SetDamageNumber(Damage);
		}
	}
}

void UEnemyManagerSubsystem::NotifyDamageMitigated(FMassEntityHandle Handle, float MitigatedAmount)
{
	AsyncTask(ENamedThreads::GameThread, [this, Handle, MitigatedAmount]()
	{
		if (int32* Slot = EntitySlotMap.Find(Handle))
		{
			MitigatedDamageAmounts[*Slot]    = MitigatedAmount;
			MitigatedDamageFadeTimers[*Slot] = MitigatedFadeDuration;
		}
	});
}

void UEnemyManagerSubsystem::ApplyDamageToEnemy(FMassEntityHandle Handle, int32 DamageThisHit, FVector ImpactLocation, EDamageType DamageType)
{
	OnEnemyDamageTaken.Broadcast(ImpactLocation, DamageThisHit);

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();

	if (EntitySubsystem)
	{
		const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
		FMassCommandBuffer& CommandBuffer = EntityManager.Defer();

		CommandBuffer.PushCommand<FMassDeferredSetCommand>(
			[Handle, DamageThisHit, DamageType](FMassEntityManager& Manager)
			{
				if (!Manager.IsEntityValid(Handle)) return;

				FDamageFragment* Damage = Manager.GetFragmentDataPtr<FDamageFragment>(Handle);
				if (Damage)
				{
					Damage->DamageAmount += DamageThisHit;
					// Keep the first damage type set this frame (don't overwrite with None)
					if (DamageType != EDamageType::Neutral)
					{
						Damage->DamageType = DamageType;
					}
				}
				else
				{
					Manager.AddFragmentToEntity(Handle, FDamageFragment::StaticStruct(),
						[DamageThisHit, DamageType](void* Fragment, const UScriptStruct&)
						{
							auto* Frag = static_cast<FDamageFragment*>(Fragment);
							Frag->DamageAmount = DamageThisHit;
							Frag->DamageType   = DamageType;
						});
				}
			}
		);
	}
}

void UEnemyManagerSubsystem::DestroyEnemyByISMC(UInstancedStaticMeshComponent* Component, int32 Item)
{
	FMassEntityHandle Handle = GetEnemyEntityHandle(Component, Item);
	if (!Handle.IsSet()) return;

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (EntitySubsystem)
	{
		const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
		EntityManager.Defer().DestroyEntity(Handle);
	}

	NotifyEnemyDeath(Handle);
}

bool UEnemyManagerSubsystem::DestroyEnemyByHit(const FHitResult& Hit)
{
	UInstancedStaticMeshComponent* HitISMC = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	if (!HitISMC || Hit.Item == INDEX_NONE) return false;

	DestroyEnemyByISMC(HitISMC, Hit.Item);
	return true;
}

void UEnemyManagerSubsystem::AssignNiagaraComponent(UNiagaraComponent* Component)
{
	NiagaraComponent = Component;
}

void UEnemyManagerSubsystem::UpdateHealthbarInformation(FHealthbarFrameData& Data)
{
	const float DeltaTime = Data.DeltaTime;

	// Zero all visibilities — only live entities get set to 1 below.
	// This ensures dead entities disappear without relying on async task timing.
	FMemory::Memzero(EnemyVisibilities.GetData(), EnemyVisibilities.Num() * sizeof(float));

	for (int32 i = 0; i < Data.Handles.Num(); i++)
	{
		FMassEntityHandle Handle = Data.Handles[i];

		if (!EntitySlotMap.Contains(Handle))
		{
			int32 Slot = FreeSlots.Num() > 0 ? FreeSlots.Pop() : EntitySlotMap.Num();
			EntitySlotMap.Add(Handle, Slot);
		}

		int32 Slot = EntitySlotMap[Handle];

		EnemyHealths[Slot]        = Data.Healths[i];
		EnemyMaxHealths[Slot]     = Data.MaxHealths[i];
		EnemyPositions[Slot]      = Data.Positions[i];
		EnemyVisibilities[Slot]   = 1.0f;
		ActiveEntityHandles[Slot] = Handle;
		EnemyVitalities[Slot]     = Data.Vitalities[i];
		BurnDurations[Slot]       = Data.BurnDurations[i];
		SlowDurations[Slot]       = Data.SlowDurations[i];
		StunDurations[Slot]       = Data.StunDurations[i];
		ModifierFlags[Slot]          = Data.ModifierFlags[i];
		FragmentedChunkSizes[Slot]   = Data.FragmentedChunkSizes[i];
	}

	// Tick mitigated damage fade
	for (int32 Slot = 0; Slot < MitigatedDamageFadeTimers.Num(); Slot++)
	{
		if (MitigatedDamageFadeTimers[Slot] > 0.f)
		{
			MitigatedDamageFadeTimers[Slot] -= DeltaTime;
			const float Alpha = FMath::Clamp(MitigatedDamageFadeTimers[Slot] / MitigatedFadeDuration, 0.f, 1.f);
			MitigatedDamageAmounts[Slot] *= Alpha;

			if (MitigatedDamageFadeTimers[Slot] <= 0.f)
			{
				MitigatedDamageFadeTimers[Slot] = 0.f;
				MitigatedDamageAmounts[Slot]    = 0.f;
			}
		}
	}

	if (!NiagaraComponent) return;

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
		NiagaraComponent, FName("Enemy Positions"), EnemyPositions);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Health"), EnemyHealths);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Max Health"), EnemyMaxHealths);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Visibilities"), EnemyVisibilities);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Vitality"), EnemyVitalities);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Burn Durations"), BurnDurations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Slow Durations"), SlowDurations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Stun Durations"), StunDurations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Modifier Flags"), ModifierFlags);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Fragmented Chunk Size"), FragmentedChunkSizes);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Mitigated Damage"), MitigatedDamageAmounts);
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

void UEnemyManagerSubsystem::NotifyHordeEnemyDeath(FName HordeID)
{
	AsyncTask(ENamedThreads::GameThread, [this, HordeID]()
	{
		OnHordeEnemyDeath.Broadcast(HordeID);
	});
}

void UEnemyManagerSubsystem::NotifyEnemyDeath(FMassEntityHandle Handle)
{
	// Called from a Mass processor which may be off the game thread.
	// Defer to game thread since broadcast subscribers touch Blueprint/timer systems.
	AsyncTask(ENamedThreads::GameThread, [this, Handle]()
	{
		if (int32* Slot = EntitySlotMap.Find(Handle))
		{
			ActiveEntityHandles[*Slot] = FMassEntityHandle();
			FreeSlots.Add(*Slot);
			EntitySlotMap.Remove(Handle);
		}

		OnEnemyDeath.Broadcast(Handle);

		if (UQuestSubsystem* QuestSubsystem = GetGameInstance()->GetSubsystem<UQuestSubsystem>())
		{
			QuestSubsystem->RegisterEnemyKilled();
		}
	});
}

void UEnemyManagerSubsystem::ApplyDamageToTarget(FMassEnemyTarget Target, int32 Damage, FVector ImpactLocation, EDamageType DamageType)
{
	ApplyDamageToEnemy(Target.EntityHandle, Damage, ImpactLocation, DamageType);
}

bool UEnemyManagerSubsystem::ApplyDamageByHit(const FHitResult& Hit, int32 Damage, EDamageType DamageType)
{
	UInstancedStaticMeshComponent* HitISMC = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	if (!HitISMC || Hit.Item == INDEX_NONE) return false;

	FMassEntityHandle Handle = GetEnemyEntityHandle(HitISMC, Hit.Item);
	if (!Handle.IsSet()) return false;

	ApplyDamageToEnemy(Handle, Damage, Hit.ImpactPoint, DamageType);
	return true;
}

static bool IsNimble(const FMassEntityManager& EntityManager, FMassEntityHandle Handle)
{
	const FModifierFragment* Mod = EntityManager.GetFragmentDataPtr<FModifierFragment>(Handle);
	return Mod && Mod->bNimble;
}

void UEnemyManagerSubsystem::ApplySlow(FMassEnemyTarget Target, float Duration, float Amount)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;
	if (IsNimble(EntityManager, Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;
	const float SpeedMultiplier = FMath::Clamp(1.f - Amount, 0.f, 1.f);

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, Duration, SpeedMultiplier](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			FSlowFragment* Slow = Manager.GetFragmentDataPtr<FSlowFragment>(Handle);
			if (Slow)
			{
				Slow->Duration        = FMath::Max(Slow->Duration, Duration);
				Slow->SpeedMultiplier = FMath::Min(Slow->SpeedMultiplier, SpeedMultiplier);
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FSlowFragment::StaticStruct(),
					[Duration, SpeedMultiplier](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FSlowFragment*>(Fragment);
						Frag->Duration        = Duration;
						Frag->SpeedMultiplier = SpeedMultiplier;
					});
			}
		});
}

void UEnemyManagerSubsystem::ApplyStun(FMassEnemyTarget Target, float Duration)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;
	if (IsNimble(EntityManager, Target.EntityHandle)) return;

	const FModifierFragment* Mod = EntityManager.GetFragmentDataPtr<FModifierFragment>(Target.EntityHandle);
	if (Mod && Mod->bDistorted) return;

	const FMassEntityHandle Handle = Target.EntityHandle;

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, Duration](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			FStunFragment* Stun = Manager.GetFragmentDataPtr<FStunFragment>(Handle);
			if (Stun)
			{
				Stun->Duration = FMath::Max(Stun->Duration, Duration);
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FStunFragment::StaticStruct(),
					[Duration](void* Fragment, const UScriptStruct&)
					{
						static_cast<FStunFragment*>(Fragment)->Duration = Duration;
					});
			}
		});
}

void UEnemyManagerSubsystem::ApplyBurn(FMassEnemyTarget Target, float Duration, float DamagePerTick, float TickInterval)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;
	if (IsNimble(EntityManager, Target.EntityHandle)) return;

	const FModifierFragment* Mod = EntityManager.GetFragmentDataPtr<FModifierFragment>(Target.EntityHandle);
	if (Mod && Mod->bPyroclastic) return;

	const FMassEntityHandle Handle = Target.EntityHandle;

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, Duration, DamagePerTick, TickInterval](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			FBurnFragment* Burn = Manager.GetFragmentDataPtr<FBurnFragment>(Handle);
			if (Burn)
			{
				Burn->Duration      = FMath::Max(Burn->Duration, Duration);
				Burn->DamagePerTick = FMath::Max(Burn->DamagePerTick, DamagePerTick);
				// Keep the shorter interval (faster ticking burn wins)
				Burn->TickInterval  = FMath::Min(Burn->TickInterval, TickInterval);
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FBurnFragment::StaticStruct(),
					[Duration, DamagePerTick, TickInterval](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FBurnFragment*>(Fragment);
						Frag->Duration      = Duration;
						Frag->DamagePerTick = DamagePerTick;
						Frag->TickInterval  = TickInterval;
						Frag->TimeToNextTick = TickInterval;
					});
			}
		});
}

bool UEnemyManagerSubsystem::CheckEnemyStealth(FMassEnemyTarget Target) const
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return false;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return false;

	const FModifierFragment* Mod = EntityManager.GetFragmentDataPtr<FModifierFragment>(Target.EntityHandle);
	return Mod && Mod->bStealthy;
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
	return (Idx != INDEX_NONE && Idx < EnemyPositions.Num()) ? EnemyPositions[Idx] + FVector(0.0f, 0.0f, 50.0f) : FVector::ZeroVector;
}

float UEnemyManagerSubsystem::GetEntityHealth(FMassEntityHandle Handle) const
{
	const int32 Idx = ActiveEntityHandles.IndexOfByKey(Handle);
	if (Idx == INDEX_NONE || Idx >= EnemyHealths.Num()) return 0.f;
	const float Max = EnemyMaxHealths[Idx];
	return (Max > 0.f) ? EnemyHealths[Idx] / Max : 0.f;
}

void UEnemyManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

FEnemyData UEnemyManagerSubsystem::GetEnemyData(const FName& EnemyID)
{
	return EnemyDataMap[EnemyID];
}
