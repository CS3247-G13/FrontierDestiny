// Fill out your copyright notice in the Description page of Project Settings.
#include "EnemyDamageMassProcessor.h"
#include "EnemyManagerSubsystem.h"
#include "HordeIDFragment.h"
#include "StatusEffectFragments.h"
#include "MassCommonFragments.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include <MassRepresentationSubsystem.h>

static constexpr float KineticDRMultiplier  = 0.5f;
static constexpr float LaserDRMultiplier    = 0.5f;
static constexpr float ElectricDRMultiplier = 0.5f;
static constexpr float AmorphicDamageCap    = 7.f;
static constexpr float FragmentedChunkSize  = 8.f;

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
	EntityQuery.AddRequirement<FModifierFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FVitalityFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);

	EntityQuery.RegisterWithProcessor(*this);
}

void UEnemyDamageMassProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Warning, TEXT("Damage Processor Ticking!"));

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [this, EnemyManager](FMassExecutionContext& Context)
		{
			TArrayView<FHealthFragment>    HealthList   = Context.GetMutableFragmentView<FHealthFragment>();
			TConstArrayView<FDamageFragment>   DamageList   = Context.GetFragmentView<FDamageFragment>();
			TConstArrayView<FHordeIDFragment>  HordeIDList  = Context.GetFragmentView<FHordeIDFragment>();
			TConstArrayView<FModifierFragment> ModifierList = Context.GetFragmentView<FModifierFragment>();
			TArrayView<FVitalityFragment>  VitalityList = Context.GetMutableFragmentView<FVitalityFragment>();

			const bool bHasModifiers = !ModifierList.IsEmpty();
			const bool bHasVitality  = !VitalityList.IsEmpty();
			const int32 NumEntities  = Context.GetNumEntities();

			for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
			{
				FHealthFragment& Health = HealthList[EntityIdx];
				const FDamageFragment& Damage = DamageList[EntityIdx];
				const FMassEntityHandle Entity = Context.GetEntity(EntityIdx);

				float FinalDamage = Damage.DamageAmount;

				if (bHasModifiers)
				{
					const FModifierFragment& Mod = ModifierList[EntityIdx];

					// Damage resistance
					if (Mod.bArmoured   && Damage.DamageType == EDamageType::Kinetic)  FinalDamage *= KineticDRMultiplier;
					if (Mod.bReflective && Damage.DamageType == EDamageType::Laser)    FinalDamage *= LaserDRMultiplier;
					if (Mod.bInsulated  && Damage.DamageType == EDamageType::Electric) FinalDamage *= ElectricDRMultiplier;

					// Amorphic: cap damage per hit at 7
					if (Mod.bAmorphic && FinalDamage > AmorphicDamageCap)
					{
						const float Mitigated = FinalDamage - AmorphicDamageCap;
						FinalDamage = AmorphicDamageCap;
						if (EnemyManager)
						{
							EnemyManager->NotifyDamageMitigated(Entity, Mitigated);
						}
					}

					// Fragmented: damage must be a multiple of 8, excess is mitigated
					if (Mod.bFragmented && FinalDamage > 0.f)
					{
						const float Floored = FMath::Floor(FinalDamage / FragmentedChunkSize) * FragmentedChunkSize;
						const float Mitigated = FinalDamage - Floored;
						FinalDamage = Floored;
						if (EnemyManager && Mitigated > 0.f)
						{
							EnemyManager->NotifyDamageMitigated(Entity, Mitigated);
						}
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

					// TEST: slow on hit for 1 second
					Context.Defer().PushCommand<FMassDeferredSetCommand>(
						[Entity](FMassEntityManager& Manager)
						{
							if (!Manager.IsEntityValid(Entity)) return;
							FSlowFragment* Slow = Manager.GetFragmentDataPtr<FSlowFragment>(Entity);
							if (Slow)
							{
								Slow->Duration = FMath::Max(Slow->Duration, 1.0f);
							}
							else
							{
								Manager.AddFragmentToEntity(Entity, FSlowFragment::StaticStruct(),
									[](void* Fragment, const UScriptStruct&)
									{
										static_cast<FSlowFragment*>(Fragment)->Duration = 1.0f;
									});
							}
						}
					);
				}
			}
		});
}