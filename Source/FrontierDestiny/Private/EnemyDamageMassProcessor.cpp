// Fill out your copyright notice in the Description page of Project Settings.
#include "EnemyDamageMassProcessor.h"
#include "EnemyManagerSubsystem.h"
#include "HordeIDFragment.h"
#include "StatusEffectFragments.h"
#include "MassCommonFragments.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include <MassRepresentationSubsystem.h>

// Required for UE 5.1+ to optimize compile times
#include UE_INLINE_GENERATED_CPP_BY_NAME(EnemyDamageMassProcessor)

UEnemyDamageMassProcessor::UEnemyDamageMassProcessor()
{
	// Standard setup for Mass processors
	bAutoRegisterWithProcessingPhases = true;
	//ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client |
		EProcessorExecutionFlags::Standalone |
		EProcessorExecutionFlags::Editor);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	// Ensures this runs relative to other representation tasks
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
}

void UEnemyDamageMassProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FDamageFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FHordeIDFragment>(EMassFragmentAccess::ReadOnly,  EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FModifierFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FVitalityFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FStatsFragment>(EMassFragmentAccess::ReadWrite,      EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FPyroclasticFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);

	EntityQuery.RegisterWithProcessor(*this);
}

void UEnemyDamageMassProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Warning, TEXT("Damage Processor Ticking!"));

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [this, EnemyManager](FMassExecutionContext& Context)
		{
			TArrayView<FHealthFragment>        HealthList   = Context.GetMutableFragmentView<FHealthFragment>();
			TConstArrayView<FDamageFragment>   DamageList   = Context.GetFragmentView<FDamageFragment>();
			TConstArrayView<FHordeIDFragment>  HordeIDList  = Context.GetFragmentView<FHordeIDFragment>();
			TArrayView<FModifierFragment>      ModifierList = Context.GetMutableFragmentView<FModifierFragment>();
			TArrayView<FVitalityFragment>      VitalityList = Context.GetMutableFragmentView<FVitalityFragment>();
			TArrayView<FStatsFragment>         StatsList       = Context.GetMutableFragmentView<FStatsFragment>();
			TArrayView<FPyroclasticFragment>   PyroclasticList = Context.GetMutableFragmentView<FPyroclasticFragment>();

			const bool bHasModifiers   = !ModifierList.IsEmpty();
			const bool bHasVitality    = !VitalityList.IsEmpty();
			const bool bHasStats       = !StatsList.IsEmpty();
			const bool bHasPyroclastic = !PyroclasticList.IsEmpty();
			const int32 NumEntities  = Context.GetNumEntities();

			for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
			{
				FHealthFragment& Health = HealthList[EntityIdx];
				const FDamageFragment& Damage = DamageList[EntityIdx];
				const FMassEntityHandle Entity = Context.GetEntity(EntityIdx);

				float FinalDamage = Damage.DamageAmount;

				if (bHasModifiers)
				{
					FModifierFragment& Mod = ModifierList[EntityIdx];

					// Damage resistance (1.0 = no change, lower = more resistant)
					if (Damage.DamageType == EDamageType::Kinetic)  FinalDamage *= Mod.KineticResistance;
					if (Damage.DamageType == EDamageType::Laser)    FinalDamage *= Mod.LaserResistance;
					if (Damage.DamageType == EDamageType::Electric) FinalDamage *= Mod.ElectricResistance;

					// Amorphic: cap damage per hit
					if (Mod.AmorphicDamageCap > 0.f && FinalDamage > Mod.AmorphicDamageCap)
					{
						const float Mitigated = FinalDamage - Mod.AmorphicDamageCap;
						FinalDamage = Mod.AmorphicDamageCap;
						if (EnemyManager)
						{
							EnemyManager->NotifyDamageMitigated(Entity, Mitigated);
						}
					}

					// Fragmented: damage floored to nearest multiple of FragmentedChunkSize
					if (Mod.FragmentedChunkSize > 0.f && FinalDamage > 0.f)
					{
						const float Floored = FMath::Floor(FinalDamage / Mod.FragmentedChunkSize) * Mod.FragmentedChunkSize;
						const float Mitigated = FinalDamage - Floored;
						FinalDamage = Floored;
						if (EnemyManager && Mitigated > 0.f)
						{
							EnemyManager->NotifyDamageMitigated(Entity, Mitigated);
						}
					}
				}

				// Pyroclastic shield — block one hit, start recharge
			if (bHasPyroclastic && FinalDamage > 0.f)
			{
				FPyroclasticFragment& Pyro = PyroclasticList[EntityIdx];
				if (Pyro.TimeToShield <= 0.f)
				{
					FinalDamage = 0.f;
					Pyro.TimeToShield = Pyro.ChargeTime;
				}
			}

			// Drain vitality before health
				if (bHasVitality && FinalDamage > 0.f)
				{
					FVitalityFragment& Vitality = VitalityList[EntityIdx];
					const float VitalityDrain = FMath::Min(Vitality.Value, FinalDamage);
					Vitality.Value -= VitalityDrain;
					FinalDamage    -= VitalityDrain;
				}

				Health.Value -= FinalDamage;

			// Stealthy: reveal on any damage that actually lands
			if (bHasModifiers && FinalDamage > 0.f)
			{
				ModifierList[EntityIdx].bStealthy = false;
			}

				if (Health.Value <= 0.f)
				{
					if (EnemyManager)
					{
						if (!HordeIDList.IsEmpty())
						{
							EnemyManager->NotifyHordeEnemyDeath(HordeIDList[EntityIdx].HordeID);
						}
						EnemyManager->NotifyEnemyDeath(Entity);
					}
					Context.Defer().DestroyEntity(Entity);
				}
				else
				{
					Context.Defer().RemoveFragment<FDamageFragment>(Entity);

					// Distorted: scale BaseSpeed based on damage taken so far
					if (bHasModifiers && bHasStats && ModifierList[EntityIdx].bDistorted)
					{
						FStatsFragment& Stats = StatsList[EntityIdx];
						const float DamageTakenRatio = 1.0f - (Health.Value / Health.MaxValue);
						Stats.BaseSpeed = Stats.InitialBaseSpeed * FMath::Lerp(1.0f, ModifierList[EntityIdx].DistortionSpeedMultiplier, DamageTakenRatio);
					}

				}
			}
		});
}
