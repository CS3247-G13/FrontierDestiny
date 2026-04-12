// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyManagerSubsystem.h"
#include "CustomChannels.h"

#include "GlobalTowerSettings.h"
#include "Async/TaskGraphInterfaces.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "MassEntitySubsystem.h"
#include "EconomySubsystem.h"
#include "MassRepresentationSubsystem.h"
#include "EnemyDamageMassProcessor.h"

#include "StatusEffectFragments.h"
#include "DamageNumber.h"

#include "QuestSubsystem.h"
#include "Engine/OverlapResult.h"

static constexpr float MitigatedFadeDuration = 1.5f;

void UEnemyManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadEnemyDataFromDataTable();
	InitializeHealthbars();

	OnEnemyDamageTaken.AddUObject(this, &UEnemyManagerSubsystem::SpawnHitEffects);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UEnemyManagerSubsystem::OnLevelChanged
	);
}


void UEnemyManagerSubsystem::OnLevelChanged(UWorld* World)
{
	EnemyHealths.Empty();
	EnemyMaxHealths.Empty();
	EnemyPositions.Empty();
	EnemyVisibilities.Empty();
	EnemyVitalities.Empty();
	BurnDurations.Empty();
	SlowDurations.Empty();
	StunDurations.Empty();
	ModifierFlags.Empty();           
	FragmentedChunkSizes.Empty();
	MitigatedDamageAmounts.Empty();

	MitigatedDamageFadeTimers.Empty();
	EnemyHeights.Empty();

	ActiveEntityHandles.Empty();
	EntitySlotMap.Empty();
	FreeSlots.Empty();

	EnemyDataMap.Empty();

	LoadEnemyDataFromDataTable();
	InitializeHealthbars();
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
	EnemyHeights.SetNum(1000);
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

	//if (UNiagaraSystem* Splatter = Settings->BloodSplatterEffect.LoadSynchronous())
	//{
	//	UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Splatter, Location);
	//}

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

void UEnemyManagerSubsystem::NotifyDamageDealt(FMassEntityHandle Handle, float FinalDamage)
{
	AsyncTask(ENamedThreads::GameThread, [this, Handle, FinalDamage]()
	{
		const FVector Location = GetEntityPosition(Handle);
		OnEnemyDamageTaken.Broadcast(Location, static_cast<int32>(FinalDamage));
	});
}

void UEnemyManagerSubsystem::ApplyDamageToEnemy(FMassEntityHandle Handle, int32 DamageThisHit, FVector ImpactLocation, EDamageType DamageType)
{

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

void UEnemyManagerSubsystem::RewardPlayerForEnemyDeath(FResourceAmount Reward)
{
	AsyncTask(ENamedThreads::GameThread, [this, Reward]()
	{
		UEconomySubsystem* ES = GetGameInstance()->GetSubsystem<UEconomySubsystem>();
		if (ES)
		{
			ES->AddFunds(Reward);
		}
	});
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

		UE_LOG(LogTemp, Display, TEXT("Checking enemy ID: %s"), *Data.EnemyIDs[i].ToString());
		if (const FEnemyData* EnemyData = EnemyDataMap.Find(Data.EnemyIDs[i]))
		{
			EnemyHeights[Slot] = EnemyData->Height;
		}
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
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName("Enemy Heights"), EnemyHeights);
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

		if (ArcChargedEntity == Handle)
		{
			ArcChargedEntity = FMassEntityHandle();
		}
		ConduitMarkedEntities.Remove(Handle);

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

void UEnemyManagerSubsystem::ApplyArcShot(FMassEnemyTarget Target, float Range, float Damage)
{
	// Find the closest already-charged entity within range
	// If there's an existing charged entity in range, arc between them
	if (ArcChargedEntity.IsSet() && ArcChargedEntity != Target.EntityHandle)
	{
		const FVector ChargedPosition = GetEntityPosition(ArcChargedEntity);
		const float DistSq = FVector::DistSquared(Target.Position, ChargedPosition);

		if (DistSq <= Range * Range)
		{
			const FMassEntityHandle PreviousHandle = ArcChargedEntity;
			ArcChargedEntity = FMassEntityHandle();

			if (UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>())
			{
				EntitySubsystem->GetEntityManager().Defer().RemoveFragment<FArcLightningFragment>(PreviousHandle);
			}

			const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
			if (UNiagaraSystem* ArcSystem = Settings->ArcLightningEffect.LoadSynchronous())
			{
				if (UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(), ArcSystem, ChargedPosition))
				{
					NiagaraComp->SetVariablePosition(FName("Origin"), ChargedPosition);
					NiagaraComp->SetVariablePosition(FName("Target"), Target.Position);
				}
			}

			ApplyDamageToEnemy(PreviousHandle,      static_cast<int32>(Damage), ChargedPosition, EDamageType::Electric);
			ApplyDamageToEnemy(Target.EntityHandle, static_cast<int32>(Damage), Target.Position,  EDamageType::Electric);
			return;
		}
	}

	// No pair in range — clear any previous charged entity and mark this one
	if (ArcChargedEntity.IsSet())
	{
		if (UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>())
		{
			EntitySubsystem->GetEntityManager().Defer().RemoveFragment<FArcLightningFragment>(ArcChargedEntity);
		}
	}

	ArcChargedEntity = Target.EntityHandle;

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;
	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, Range, Damage](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			if (!Manager.GetFragmentDataPtr<FArcLightningFragment>(Handle))
			{
				Manager.AddFragmentToEntity(Handle, FArcLightningFragment::StaticStruct(),
					[Range, Damage](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FArcLightningFragment*>(Fragment);
						Frag->Range  = Range;
						Frag->Damage = Damage;
					});
			}
		});
}

void UEnemyManagerSubsystem::ApplyConduitMarker(FMassEnemyTarget Target, float Duration, float Range, float ArcDamage)
{
	const FVector TargetPosition = GetEntityPosition(Target.EntityHandle);
	const float RangeSq = Range * Range;

	// Arc to all already-marked enemies in range
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UNiagaraSystem* ArcSystem = Settings->ArcLightningEffect.LoadSynchronous();

	for (const FMassEntityHandle& MarkedHandle : ConduitMarkedEntities)
	{
		if (MarkedHandle == Target.EntityHandle) continue;
		const FVector MarkedPosition = GetEntityPosition(MarkedHandle);
		if (FVector::DistSquared(TargetPosition, MarkedPosition) > RangeSq) continue;

		if (ArcSystem)
		{
			if (UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), ArcSystem, MarkedPosition))
			{
				NiagaraComp->SetVariablePosition(FName("Origin"), MarkedPosition);
				NiagaraComp->SetVariablePosition(FName("Target"),   TargetPosition);
			}
		}

		ApplyDamageToEnemy(MarkedHandle,        static_cast<int32>(ArcDamage), MarkedPosition,  EDamageType::Electric);
		ApplyDamageToEnemy(Target.EntityHandle, static_cast<int32>(ArcDamage), TargetPosition,  EDamageType::Electric);
	}

	// Add or refresh the marker fragment on the target
	ConduitMarkedEntities.Add(Target.EntityHandle);

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;
	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, Duration, Range, ArcDamage](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			FConduitMarkerFragment* Existing = Manager.GetFragmentDataPtr<FConduitMarkerFragment>(Handle);
			if (Existing)
			{
				Existing->Duration = FMath::Max(Existing->Duration, Duration);
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FConduitMarkerFragment::StaticStruct(),
					[Duration, Range, ArcDamage](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FConduitMarkerFragment*>(Fragment);
						Frag->Duration  = Duration;
						Frag->Range     = Range;
						Frag->ArcDamage = ArcDamage;
					});
			}
		});
}

void UEnemyManagerSubsystem::NotifyConduitMarkerExpired(FMassEntityHandle Handle)
{
	AsyncTask(ENamedThreads::GameThread, [this, Handle]()
	{
		ConduitMarkedEntities.Remove(Handle);
	});
}

void UEnemyManagerSubsystem::ApplyCompoundingInjury(FMassEnemyTarget Target, float DamagePerStack, float MaxBonus, float TimeWindow)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;
	const int32 MaxShots = FMath::FloorToInt(MaxBonus / DamagePerStack);

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, DamagePerStack, MaxBonus, TimeWindow, MaxShots](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			FCompoundingInjuryFragment* Existing = Manager.GetFragmentDataPtr<FCompoundingInjuryFragment>(Handle);
			if (Existing)
			{
				Existing->Shots = FMath::Min(Existing->Shots + 1, MaxShots);
				Existing->RemainingTime = TimeWindow;
				Existing->bAppliedThisFrame = true;
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FCompoundingInjuryFragment::StaticStruct(),
					[DamagePerStack, MaxBonus, TimeWindow](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FCompoundingInjuryFragment*>(Fragment);
						Frag->Shots             = 1;
						Frag->RemainingTime     = TimeWindow;
						Frag->bAppliedThisFrame = true;
						Frag->DamagePerStack    = DamagePerStack;
						Frag->MaxBonus          = MaxBonus;
						Frag->TimeWindow        = TimeWindow;
					});
			}
		});
}

void UEnemyManagerSubsystem::ApplySuppressed(FMassEnemyTarget Target, int32 ShotThreshold, float TimeWindow, float SlowAmount, float SlowDuration)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;
	if (IsNimble(EntityManager, Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, ShotThreshold, TimeWindow, SlowAmount, SlowDuration](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;

			int32 CurrentShots = 0;
			FSuppressedFragment* Existing = Manager.GetFragmentDataPtr<FSuppressedFragment>(Handle);
			if (Existing)
			{
				Existing->RemainingTime = TimeWindow;
				Existing->Shots++;
				CurrentShots = Existing->Shots;
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FSuppressedFragment::StaticStruct(),
					[ShotThreshold, TimeWindow, SlowAmount, SlowDuration](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FSuppressedFragment*>(Fragment);
						Frag->RemainingTime = TimeWindow;
						Frag->Shots         = 1;
						Frag->ShotThreshold = ShotThreshold;
						Frag->SlowAmount    = SlowAmount;
						Frag->SlowDuration  = SlowDuration;
					});
				CurrentShots = 1;
			}

			if (CurrentShots < ShotThreshold) return;

			const float SpeedMultiplier = FMath::Clamp(1.f - SlowAmount, 0.f, 1.f);
			FSlowFragment* Slow = Manager.GetFragmentDataPtr<FSlowFragment>(Handle);
			if (Slow)
			{
				Slow->Duration        = FMath::Max(Slow->Duration, SlowDuration);
				Slow->SpeedMultiplier = FMath::Min(Slow->SpeedMultiplier, SpeedMultiplier);
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FSlowFragment::StaticStruct(),
					[SlowDuration, SpeedMultiplier](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FSlowFragment*>(Fragment);
						Frag->Duration        = SlowDuration;
						Frag->SpeedMultiplier = SpeedMultiplier;
					});
			}
		});
}

void UEnemyManagerSubsystem::ApplyRuptured(FMassEnemyTarget Target, float Damage, float Radius)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, Damage, Radius](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			if (!Manager.GetFragmentDataPtr<FRupturedFragment>(Handle))
			{
				Manager.AddFragmentToEntity(Handle, FRupturedFragment::StaticStruct(),
					[Damage, Radius](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FRupturedFragment*>(Fragment);
						Frag->Damage = Damage;
						Frag->Radius = Radius;
					});
			}
		});
}

void UEnemyManagerSubsystem::ApplyDevastatingBlow(FMassEnemyTarget Target, float HPThreshold, float Multiplier)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, HPThreshold, Multiplier](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			FDevastatedFragment* Existing = Manager.GetFragmentDataPtr<FDevastatedFragment>(Handle);
			if (Existing)
			{
				Existing->HPThreshold = HPThreshold;
				Existing->Multiplier  = Multiplier;
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FDevastatedFragment::StaticStruct(),
					[HPThreshold, Multiplier](void* Fragment, const UScriptStruct&)
					{
						auto* Frag = static_cast<FDevastatedFragment*>(Fragment);
						Frag->HPThreshold = HPThreshold;
						Frag->Multiplier  = Multiplier;
					});
			}
		});
}

void UEnemyManagerSubsystem::ApplyBallisticRecall(FMassEnemyTarget Target, int32 AmmoRegain)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Target.EntityHandle)) return;

	const FMassEntityHandle Handle = Target.EntityHandle;

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle, AmmoRegain](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Handle)) return;
			FBallisticRecallFragment* Existing = Manager.GetFragmentDataPtr<FBallisticRecallFragment>(Handle);
			if (Existing)
			{
				Existing->AmmoRegain = AmmoRegain;
			}
			else
			{
				Manager.AddFragmentToEntity(Handle, FBallisticRecallFragment::StaticStruct(),
					[AmmoRegain](void* Fragment, const UScriptStruct&)
					{
						static_cast<FBallisticRecallFragment*>(Fragment)->AmmoRegain = AmmoRegain;
					});
			}
		});
}

void UEnemyManagerSubsystem::Rupture(FVector Position, float Damage, float Radius)
{
	AsyncTask(ENamedThreads::GameThread, [this, Position, Damage, Radius]()
	{
		TArray<FMassEntityHandle> NearbyHandles;
		GetEntitiesInRange(Position, Radius, NearbyHandles);
		for (const FMassEntityHandle& Handle : NearbyHandles)
		{
			ApplyDamageToEnemy(Handle, static_cast<int32>(Damage), Position, EDamageType::Kinetic);
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
	OutTarget.Position = Hit.ImpactPoint;
	OutTarget.EnemyID = GetEntityEnemyID(Handle);
	return true;
}

FName UEnemyManagerSubsystem::GetEntityEnemyID(FMassEntityHandle Handle) const
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return NAME_None;

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	if (!EntityManager.IsEntityValid(Handle)) return NAME_None;

	const FStatsFragment* Stats = EntityManager.GetFragmentDataPtr<FStatsFragment>(Handle);
	return Stats ? Stats->EnemyID : NAME_None;
}

void UEnemyManagerSubsystem::GetEntitiesInRange(FVector Center, float Radius, TArray<FMassEntityHandle>& OutHandles) const
{
	const float RadiusSq = Radius * Radius;
	for (int32 i = 0; i < ActiveEntityHandles.Num(); i++)
	{
		const FMassEntityHandle& Handle = ActiveEntityHandles[i];
		if (!Handle.IsSet()) continue;
		if (FVector::DistSquared(EnemyPositions[i], Center) <= RadiusSq)
			OutHandles.Add(Handle);
	}
}

FVector UEnemyManagerSubsystem::GetEntityPosition(FMassEntityHandle Handle) const
{
	const int32 Idx = ActiveEntityHandles.IndexOfByKey(Handle);
	return (Idx != INDEX_NONE && Idx < EnemyPositions.Num()) ? EnemyPositions[Idx] + FVector(0.f, 0.f, EnemyHeights[Idx] * 0.5f) : FVector::ZeroVector;
}

FVector UEnemyManagerSubsystem::GetTargetPosition(FMassEnemyTarget Target) const
{
	return GetEntityPosition(Target.EntityHandle);
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

bool UEnemyManagerSubsystem::IsEnhancedEnemy(FMassEntityHandle Handle) const
{
	const FName EnemyID = GetEntityEnemyID(Handle);
	if (EnemyID == NAME_None) return false;

	const FEnemyData* Data = EnemyDataMap.Find(EnemyID);
	return Data && Data->bIsEnhanced;
}
