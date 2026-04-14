// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyLifetimeProcessor.h"
#include "StatusEffectFragments.h"
#include "EnemyDamageMassProcessor.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include "MassRepresentationSubsystem.h"

UEnemyLifetimeProcessor::UEnemyLifetimeProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client |
		EProcessorExecutionFlags::Standalone |
		EProcessorExecutionFlags::Editor);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
}

void UEnemyLifetimeProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	LifetimeQuery.Initialize(EntityManager);
	LifetimeQuery.AddRequirement<FLifetimeFragment>(EMassFragmentAccess::ReadWrite);
	LifetimeQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadWrite);
	LifetimeQuery.RegisterWithProcessor(*this);
}

void UEnemyLifetimeProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	LifetimeQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FLifetimeFragment> LifetimeList = Context.GetMutableFragmentView<FLifetimeFragment>();
		TArrayView<FHealthFragment>   HealthList   = Context.GetMutableFragmentView<FHealthFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			FLifetimeFragment& Lifetime = LifetimeList[i];
			Lifetime.RemainingTime -= DeltaTime;

			if (Lifetime.RemainingTime <= 0.f)
			{
				HealthList[i].Value = 0.f;

				const FMassEntityHandle Entity = Context.GetEntity(i);
				Context.Defer().PushCommand<FMassDeferredSetCommand>(
					[Entity](FMassEntityManager& Manager)
					{
						if (!Manager.IsEntityValid(Entity)) return;
						Manager.AddFragmentToEntity(Entity, FDamageFragment::StaticStruct(),
							[](void* Fragment, const UScriptStruct&)
							{
								static_cast<FDamageFragment*>(Fragment)->DamageAmount = 9999.f;
							});
					});
			}
		}
	});
}
