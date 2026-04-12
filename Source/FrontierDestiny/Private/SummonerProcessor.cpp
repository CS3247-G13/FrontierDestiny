// Fill out your copyright notice in the Description page of Project Settings.


#include "SummonerProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include <MassRepresentationSubsystem.h>
#include <MassSpawnerSubsystem.h>
#include <EnemyManagerSubsystem.h>
#include <MassSpawner.h>
#include "MassSpawnerTypes.h"       // For FMassEntitySpawnData
#include "MassSpawnerSubsystem.h"   // For UMassSpawnerSubsystem
#include "EnemyManagerSubsystem.h"  // For FEnemyData, GetEnemyData
#include "MassEntityConfigAsset.h"  // For UMassEntityConfigAsset
#include <NavigationSystem.h>

USummonerProcessor::USummonerProcessor()
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

void USummonerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	EntityQuery.AddRequirement<FSummonerFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.RegisterWithProcessor(*this);

	EQ2.Initialize(EntityManager);
    // Inside USummonerProcessor::ConfigureQueries()
    //EQ2.AddRequirement<FSummonerFragment>(EMassFragmentAccess::ReadWrite);
    EQ2.AddRequirement<FSummonRequestFragment>(EMassFragmentAccess::ReadWrite);
	EQ2.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);

	EQ2.RegisterWithProcessor(*this);
}


void USummonerProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    //FMassCommandBuffer& CommandBuffer = EntityManager.Defer();

    EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
        {
            const int32 NumEntities = Context.GetNumEntities();

            auto Summoners = Context.GetMutableFragmentView<FSummonerFragment>();
            auto Entities = Context.GetEntities();
            FMassCommandBuffer& CommandBuffer = Context.Defer();

            for (int32 i = 0; i < NumEntities; i++)
            {
                FSummonerFragment& Summoner = Summoners[i];
                const FMassEntityHandle Entity = Entities[i];

                // Tick timer
                Summoner.TimeRemaining -= DeltaTime;

                // Not ready yet
                if (Summoner.TimeRemaining > 0.f)
                {
                    continue;
                }

                // Reset timer (use += to avoid drift on frame spikes)
                Summoner.TimeRemaining += Summoner.Cooldown;

                const int32 Count = Summoner.UnitsPerSummon;
                const float Radius = Summoner.SpawnRadius;
                const FName EnemyID = Summoner.EnemyID; // <- Added back

                // Defer: Add or update summon request fragment
                CommandBuffer.PushCommand<FMassDeferredSetCommand>(
                    [Entity, Count, Radius, EnemyID](FMassEntityManager& Manager)
                    {
                        if (!Manager.IsEntityValid(Entity)) return;

                        FSummonRequestFragment* Request =
                            Manager.GetFragmentDataPtr<FSummonRequestFragment>(Entity);

                        if (Request)
                        {
                            // Accumulate if already requested this frame
                            Request->Count += Count;
                        }
                        else
                        {
                            Manager.AddFragmentToEntity(Entity, FSummonRequestFragment::StaticStruct(),
                                [Count, Radius, EnemyID](void* Fragment, const UScriptStruct&)
                                {
                                    auto* Frag = static_cast<FSummonRequestFragment*>(Fragment);
                                    Frag->Count = Count;
                                    Frag->Radius = Radius;
                                    Frag->EnemyID = EnemyID; // <- Added here
                                });
                        }
                    }
                );
            }
        });

    EQ2.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
        {
            const int32 NumEntities = Context.GetNumEntities();

            auto Requests = Context.GetMutableFragmentView<FSummonRequestFragment>();
            auto Transforms = Context.GetFragmentView<FTransformFragment>();
            auto Entities = Context.GetEntities();

            UWorld* World = Context.GetWorld();
            if (!World) return;

            UEnemyManagerSubsystem* Subsystem =
                World->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();

            if (!Subsystem) return;

            for (int32 i = 0; i < NumEntities; i++)
            {
                FSummonRequestFragment& Request = Requests[i];
                if (Request.Count <= 0)
                    continue;

                const FMassEntityHandle Entity = Entities[i];
                const FTransform& BaseTransform = Transforms[i].GetTransform();

                // ----------------------------
                // QUEUE SPAWN ONLY
                // ----------------------------
                FEnemySpawnRequest SpawnReq;
                SpawnReq.SourceEntity = Entity;
                SpawnReq.EnemyID = Request.EnemyID;
                SpawnReq.BaseTransform = BaseTransform;
                SpawnReq.Count = Request.Count;
                SpawnReq.Radius = Request.Radius;

                Subsystem->QueueSpawnRequest(SpawnReq);
            }

            // ----------------------------
            // REMOVE REQUEST FRAGMENTS
            // ----------------------------
            for (int32 i = 0; i < NumEntities; i++)
            {
                if (Requests[i].Count > 0)
                {
                    Context.Defer().RemoveFragment<FSummonRequestFragment>(Entities[i]);
                }
            }
        });
}
