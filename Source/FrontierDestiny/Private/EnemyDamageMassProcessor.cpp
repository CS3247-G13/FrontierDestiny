// Fill out your copyright notice in the Description page of Project Settings.
#include "EnemyDamageMassProcessor.h"
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

	EntityQuery.RegisterWithProcessor(*this);
}

void UEnemyDamageMassProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Warning, TEXT("Damage Processor Ticking!"));
	// The Lambda version of ForEachEntityChunk is the standard modern pattern
	EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& Context)
		{
			TArrayView<FHealthFragment> HealthList = Context.GetMutableFragmentView<FHealthFragment>();
			TConstArrayView<FDamageFragment> DamageList = Context.GetFragmentView<FDamageFragment>();
			const int32 NumEntities = Context.GetNumEntities();


			for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
			{
				FHealthFragment& Health = HealthList[EntityIdx];
				const FDamageFragment& Damage = DamageList[EntityIdx];

				// Apply damage logic
				Health.Value -= Damage.DamageAmount;

				const FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
				if (Health.Value <= 0.f)
				{
					Context.Defer().DestroyEntity(Entity);
				}
				else
				{
					// Remove the damage fragment so it doesn't re-trigger next frame
					Context.Defer().RemoveFragment<FDamageFragment>(Entity);
				}
			}
		});
}