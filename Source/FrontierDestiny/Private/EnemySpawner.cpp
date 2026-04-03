// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemySpawner.h"
#include "NativeGameplayTags.h"
#include "CoreManagerSubsystem.h"
#include "EnemyManagerSubsystem.h"
#include "HordeIDFragment.h"
#include "MassEntitySubsystem.h"
#include "MassCommandBuffer.h"
#include "EnemyDamageMassProcessor.h"
#include "Kismet/GameplayStatics.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Fast,         "Enemy.Modifier.Fast")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Strong,       "Enemy.Modifier.Strong")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Vitality,     "Enemy.Modifier.Vitality")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Armoured,     "Enemy.Modifier.Armoured")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Reflective,   "Enemy.Modifier.Reflective")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Insulated,    "Enemy.Modifier.Insulated")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Stealthy,     "Enemy.Modifier.Stealthy")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Nimble,       "Enemy.Modifier.Nimble")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Pyroclastic,  "Enemy.Modifier.Pyroclastic")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Amorphic,     "Enemy.Modifier.Amorphic")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Distorted,    "Enemy.Modifier.Distorted")
UE_DEFINE_GAMEPLAY_TAG(TAG_Modifier_Fragmented,   "Enemy.Modifier.Fragmented")

static FModifierFragment BuildModifierFragment(const FEnemySpawnEntry& Entry)
{
	const FGameplayTagContainer& Tags = Entry.Modifiers;
	FModifierFragment Mod;

	Mod.bFast        = Tags.HasTag(TAG_Modifier_Fast);
	Mod.bStrong      = Tags.HasTag(TAG_Modifier_Strong);
	Mod.bVitality    = Tags.HasTag(TAG_Modifier_Vitality);
	Mod.bArmoured    = Tags.HasTag(TAG_Modifier_Armoured);
	Mod.bReflective  = Tags.HasTag(TAG_Modifier_Reflective);
	Mod.bInsulated   = Tags.HasTag(TAG_Modifier_Insulated);
	Mod.bStealthy    = Tags.HasTag(TAG_Modifier_Stealthy);
	Mod.bNimble      = Tags.HasTag(TAG_Modifier_Nimble);
	Mod.bPyroclastic = Tags.HasTag(TAG_Modifier_Pyroclastic);
	Mod.bAmorphic    = Tags.HasTag(TAG_Modifier_Amorphic);
	Mod.bDistorted   = Tags.HasTag(TAG_Modifier_Distorted);
	Mod.bFragmented  = Tags.HasTag(TAG_Modifier_Fragmented);

	Mod.KineticResistance         = Mod.bArmoured    ? 0.5f : Entry.KineticResistance;
	Mod.LaserResistance           = Mod.bReflective  ? 0.5f : Entry.LaserResistance;
	Mod.ElectricResistance        = Mod.bInsulated   ? 0.5f : Entry.ElectricResistance;
	Mod.AmorphicDamageCap         = Mod.bAmorphic   ? Entry.AmorphicDamageCap         : 0.f;
	Mod.FragmentedChunkSize       = Mod.bFragmented  ? Entry.FragmentedChunkSize        : 0.f;
	Mod.DistortionSpeedMultiplier = Mod.bDistorted   ? Entry.DistortionSpeedMultiplier  : 1.5f;

	return Mod;
}

AEnemySpawner::AEnemySpawner()
{
	bAutoSpawnOnBeginPlay = false;
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (UEnemyWaveManagerSubsystem* WaveManager = GetGameInstance()->GetSubsystem<UEnemyWaveManagerSubsystem>())
	{
		WaveManager->OnSpawnOrderIssued.AddDynamic(this, &AEnemySpawner::HandleSpawnOrder);
		WaveManager->OnHordeBatchBegin.AddDynamic(this, &AEnemySpawner::HandleHordeBatchBegin);
		UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Subscribed to OnSpawnOrderIssued (Tag='%s', CoreIndex=%d, bIsBackup=%d)"),
			*GetName(), *Tag.ToString(), CoreIndex, bIsBackup);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: Could not find EnemyWaveManagerSubsystem — spawn orders will not be received."), *GetName());
	}

	OnSpawningFinishedEvent.AddDynamic(this, &AEnemySpawner::HandleSpawningFinished);
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
			if (UEnemyWaveManagerSubsystem* WaveManager = GetGameInstance()->GetSubsystem<UEnemyWaveManagerSubsystem>())
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
	PendingModifierFragments.Empty();
	PendingHasModifiers.Empty();
	PendingBaseHP.Empty();
	PendingBaseSpeed.Empty();
	PendingBaseDamage.Empty();
	PendingSpeedMultipliers.Empty();
	PendingDamageMultipliers.Empty();
	PendingVitalityAmounts.Empty();
	PendingEnemyIDs.Empty();
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

		PendingBaseHP.Add(EnemyData.Attributes.Contains("HP")     ? EnemyData.Attributes["HP"].BaseValue     : 20.f);
		PendingBaseSpeed.Add(EnemyData.Attributes.Contains("Speed")  ? EnemyData.Attributes["Speed"].BaseValue  : 1.f);
		PendingBaseDamage.Add(EnemyData.Attributes.Contains("Damage") ? EnemyData.Attributes["Damage"].BaseValue : 10.f);

		const FModifierFragment ModFrag = BuildModifierFragment(EnemyInfo);
		PendingModifierFragments.Add(ModFrag);
		PendingHasModifiers.Add(!EnemyInfo.Modifiers.IsEmpty());
		PendingSpeedMultipliers.Add(ModFrag.bFast     ? EnemyInfo.FastSpeedMultiplier    : 1.f);
		PendingDamageMultipliers.Add(ModFrag.bStrong  ? EnemyInfo.StrongDamageMultiplier : 1.f);
		PendingVitalityAmounts.Add(ModFrag.bVitality  ? EnemyInfo.VitalityAmount         : 0.f);
		PendingEnemyIDs.Add(EnemyInfo.EnemyID);
	}

	Count = TotalCount;
	SpawnStartIndex = AllSpawnedEntities.Num();
	UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Calling DoSpawning()."), *GetName());
	DoSpawning();
}

void AEnemySpawner::HandleHordeBatchBegin(FName HordeID)
{
	CurrentHordeID = HordeID;
}

void AEnemySpawner::HandleSpawningFinished()
{
	if (CurrentHordeID.IsNone())
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassCommandBuffer& CommandBuffer = EntitySubsystem->GetEntityManager().Defer();
	const FName HordeID = CurrentHordeID;

	int32 TotalStamped = 0;
	for (int32 i = SpawnStartIndex; i < AllSpawnedEntities.Num(); i++)
	{
		TotalStamped += AllSpawnedEntities[i].Entities.Num();

		const int32 ModIdx = i - SpawnStartIndex;
		const bool bApplyModifiers = PendingHasModifiers.IsValidIndex(ModIdx) && PendingHasModifiers[ModIdx];
		const FModifierFragment ModFrag        = bApplyModifiers ? PendingModifierFragments[ModIdx] : FModifierFragment{};
		const float BaseHP                     = PendingBaseHP.IsValidIndex(ModIdx)            ? PendingBaseHP[ModIdx]            : 20.f;
		const float BaseSpeed                  = PendingBaseSpeed.IsValidIndex(ModIdx)         ? PendingBaseSpeed[ModIdx]         : 1.f;
		const float BaseDamage                 = PendingBaseDamage.IsValidIndex(ModIdx)        ? PendingBaseDamage[ModIdx]        : 10.f;
		const float SpeedMultiplier            = PendingSpeedMultipliers.IsValidIndex(ModIdx)  ? PendingSpeedMultipliers[ModIdx]  : 1.f;
		const float DamageMultiplier           = PendingDamageMultipliers.IsValidIndex(ModIdx) ? PendingDamageMultipliers[ModIdx] : 1.f;
		const float VitalityAmount             = PendingVitalityAmounts.IsValidIndex(ModIdx)   ? PendingVitalityAmounts[ModIdx]   : 0.f;
		const FName EnemyID                    = PendingEnemyIDs.IsValidIndex(ModIdx)          ? PendingEnemyIDs[ModIdx]           : NAME_None;

		for (const FMassEntityHandle& Entity : AllSpawnedEntities[i].Entities)
		{
			CommandBuffer.PushCommand<FMassDeferredSetCommand>(
				[Entity, HordeID, bApplyModifiers, ModFrag, BaseHP, BaseSpeed, BaseDamage, SpeedMultiplier, DamageMultiplier, VitalityAmount, EnemyID](FMassEntityManager& Manager)
				{
					if (!Manager.IsEntityValid(Entity)) return;

					Manager.AddFragmentToEntity(Entity, FHordeIDFragment::StaticStruct(),
						[HordeID](void* Fragment, const UScriptStruct&)
						{
							static_cast<FHordeIDFragment*>(Fragment)->HordeID = HordeID;
						});

					FHealthFragment* Health = Manager.GetFragmentDataPtr<FHealthFragment>(Entity);
					if (Health)
					{
						Health->Value    = BaseHP;
						Health->MaxValue = BaseHP;
					}

					FStatsFragment* Stats = Manager.GetFragmentDataPtr<FStatsFragment>(Entity);
					if (Stats)
					{
						Stats->BaseSpeed        = BaseSpeed * SpeedMultiplier;
						Stats->InitialBaseSpeed = Stats->BaseSpeed;
						Stats->BaseDamage       = BaseDamage * DamageMultiplier;
						Stats->EnemyID          = EnemyID;

						// Snapshot after multipliers so distortion ramps from the boosted speed
						if (ModFrag.bDistorted)
						{
							Stats->InitialBaseSpeed = Stats->BaseSpeed;
						}
					}

					if (bApplyModifiers)
					{
						Manager.AddFragmentToEntity(Entity, FModifierFragment::StaticStruct(),
							[ModFrag](void* Fragment, const UScriptStruct&)
							{
								*static_cast<FModifierFragment*>(Fragment) = ModFrag;
							});

						if (ModFrag.bPyroclastic)
						{
							Manager.AddFragmentToEntity(Entity, FPyroclasticFragment::StaticStruct(),
								[](void*, const UScriptStruct&){});
						}

						if (VitalityAmount > 0.f)
						{
							Manager.AddFragmentToEntity(Entity, FVitalityFragment::StaticStruct(),
								[VitalityAmount](void* Fragment, const UScriptStruct&)
								{
									auto* Frag = static_cast<FVitalityFragment*>(Fragment);
									Frag->Value    = VitalityAmount;
									Frag->MaxValue = VitalityAmount;
								});
						}
					}
				});
		}
	}

	if (UEnemyWaveManagerSubsystem* WaveManager = GetGameInstance()->GetSubsystem<UEnemyWaveManagerSubsystem>())
	{
		WaveManager->RegisterSpawnedEnemies(HordeID, TotalStamped);
	}
}
