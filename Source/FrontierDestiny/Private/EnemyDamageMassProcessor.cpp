// Fill out your copyright notice in the Description page of Project Settings.
#include "EnemyDamageMassProcessor.h"
#include "EnemyManagerSubsystem.h"
#include "HordeIDFragment.h"
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

	// ReadWrite access for Health since we decrease it
	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadWrite);

	// ReadOnly for Damage as we only need its value
	EntityQuery.AddRequirement<FDamageFragment>(EMassFragmentAccess::ReadOnly);

	// Optional — only present on enemies spawned via the wave system
	EntityQuery.AddRequirement<FHordeIDFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);

	EntityQuery.RegisterWithProcessor(*this);
}

void UEnemyDamageMassProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Warning, TEXT("Damage Processor Ticking!"));

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [this, EnemyManager](FMassExecutionContext& Context)
		{
			TArrayView<FHealthFragment> HealthList = Context.GetMutableFragmentView<FHealthFragment>();
			TConstArrayView<FDamageFragment> DamageList = Context.GetFragmentView<FDamageFragment>();
			TConstArrayView<FHordeIDFragment> HordeIDList = Context.GetFragmentView<FHordeIDFragment>();
			const int32 NumEntities = Context.GetNumEntities();

			for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
			{
				FHealthFragment& Health = HealthList[EntityIdx];
				const FDamageFragment& Damage = DamageList[EntityIdx];

				Health.Value -= Damage.DamageAmount;

				const FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
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
				}
			}
		});
}