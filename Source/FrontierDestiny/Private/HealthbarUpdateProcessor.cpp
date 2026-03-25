// Fill out your copyright notice in the Description page of Project Settings.

#include "MassCommonFragments.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include <MassRepresentationSubsystem.h>
#include "EnemyManagerSubsystem.h"

#include "HealthbarUpdateProcessor.h"

UHealthbarUpdateProcessor::UHealthbarUpdateProcessor()
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
	ExecutionOrder.ExecuteAfter.Add(UEnemyDamageMassProcessor::StaticClass()->GetFName());
}

void UHealthbarUpdateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	// ReadOnly access to the health fragment
	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.RegisterWithProcessor(*this);
}

void UHealthbarUpdateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI) return;
	UEnemyManagerSubsystem* Subsystem = GI->GetSubsystem<UEnemyManagerSubsystem>();
	if (!Subsystem) return;

	UE_LOG(LogTemp, Warning, TEXT("Healthbar Processor Ticking!"));
	
	TArray<float> HealthRatios;
	TArray<FVector> Positions;
	TArray<FMassEntityHandle> EntityHandles;

	// The Lambda version of ForEachEntityChunk is the standard modern pattern
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		TConstArrayView<FHealthFragment> HealthList = Context.GetFragmentView<FHealthFragment>();
		TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
		{
			Positions.Add(TransformList[EntityIdx].GetTransform().GetLocation());
			HealthRatios.Add(HealthList[EntityIdx].Value / HealthList[EntityIdx].MaxValue);
			EntityHandles.Add(Context.GetEntity(EntityIdx));
		}
	});

	Subsystem->UpdateHealthbarInformation(HealthRatios, Positions, EntityHandles);
}
