// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyNavMeshGroundingProcessor.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "NavigationSystem.h"
#include "MassExecutionContext.h"

UEnemyNavMeshGroundingProcessor::UEnemyNavMeshGroundingProcessor()
{

	bAutoRegisterWithProcessingPhases = true;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
}

void UEnemyNavMeshGroundingProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);

	EntityQuery.RegisterWithProcessor(*this);
}

void UEnemyNavMeshGroundingProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return;
	}

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& Context)
		{
			const float DeltaTime = Context.GetDeltaTimeSeconds();

			auto Transforms = Context.GetMutableFragmentView<FTransformFragment>();
			const auto Velocities = Context.GetFragmentView<FMassVelocityFragment>();

			for (int32 i = 0; i < Context.GetNumEntities(); i++)
			{
				FTransform& Transform = Transforms[i].GetMutableTransform();
				const FVector Velocity = Velocities[i].Value;

				// Integrate movement
				FVector NewLocation = Transform.GetLocation() + Velocity * DeltaTime;

				// Project to NavMesh
				FNavLocation ProjectedLocation;
				const FVector Extent(50.f, 50.f, 200.f);

				if (NavSys->ProjectPointToNavigation(NewLocation, ProjectedLocation, Extent))
				{
					NewLocation.Z = ProjectedLocation.Location.Z;
				}

				Transform.SetLocation(NewLocation);
			}
		});
}
