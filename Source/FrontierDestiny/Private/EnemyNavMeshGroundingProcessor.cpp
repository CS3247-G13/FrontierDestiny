// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyNavMeshGroundingProcessor.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "NavigationSystem.h"
#include "MassExecutionContext.h"



#include "WaterSplineComponent.h"

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

	// Temporary water detection
	// Skip if no water bodies cached
    if (CachedWaterBodies.IsEmpty()) return;

    UWaterSubsystem* WaterSubsystem = UWaterSubsystem::GetWaterSubsystem(World);
    if (!WaterSubsystem) return;

    const EWaterBodyQueryFlags QueryFlags =
        EWaterBodyQueryFlags::ComputeLocation |
        EWaterBodyQueryFlags::ComputeImmersionDepth;

    EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& ChunkContext)
        {
            auto Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();

            for (int32 i = 0; i < ChunkContext.GetNumEntities(); i++)
            {
                const FVector EntityLocation = Transforms[i].GetTransform().GetLocation();

                FWaterBodyQueryResult BestResult;
                float BestDepth = -FLT_MAX;
                UWaterBodyComponent* BestBody = nullptr;

                for (UWaterBodyComponent* Body : CachedWaterBodies)
                {
                    if (!Body) continue;

                    // Only process lakes
                    if (Body->GetWaterBodyType() != EWaterBodyType::Lake)
                        continue;

                    // -----------------------------
                    // -----------------------------
                    const FBox LakeBounds = Body->GetCollisionComponentBounds();
                    if (!LakeBounds.IsInsideXY(EntityLocation))
                        continue; // Skip if entity is outside the XY bounds

                    // Optional: further restrict by distance to lake center
                    const FVector LakeCenter = LakeBounds.GetCenter();
                    const float DistSqr = FVector::DistSquared2D(EntityLocation, LakeCenter);
                    constexpr float MaxLakeDistance = 5000.0f; // adjust per lake size
                    if (DistSqr > FMath::Square(MaxLakeDistance))
                        continue;

                    // -----------------------------
                    // -----------------------------
                    auto Query = Body->TryQueryWaterInfoClosestToWorldLocation(
                        EntityLocation,
                        QueryFlags,
                        {} // No spline needed for lakes
                    );

                    if (!Query.HasValue())
                    {
#if WITH_EDITOR
                        UE_LOG(LogTemp, Warning,
                            TEXT("Water query failed at (%f, %f, %f) for lake %s, error: %d"),
                            EntityLocation.X, EntityLocation.Y, EntityLocation.Z,
                            *Body->GetOwner()->GetActorLabel(),
                            (int32)Query.GetError());
#endif
                        continue; // skip failed queries
                    }

                    const FWaterBodyQueryResult& Result = Query.GetValue();

#if WITH_EDITOR
                    // Log water query location
                    const FVector QueryLoc = Result.GetWaterSurfaceLocation();
                    UE_LOG(LogTemp, Warning,
                        TEXT("Water query location: (%f, %f, %f) for lake %s"),
                        QueryLoc.X, QueryLoc.Y, QueryLoc.Z,
                        *Body->GetOwner()->GetActorLabel());
#endif

                    // -----------------------------
                    // -----------------------------
                    if (!Result.IsInWater())
                        continue;

                    const float Depth = Result.GetImmersionDepth();
                    if (Depth > BestDepth)
                    {
                        BestDepth = Depth;
                        BestResult = Result;
                        BestBody = Body;
                    }
                }

                if (!BestBody)
                    continue;

#if WITH_EDITOR
                UE_LOG(LogTemp, Warning,
                    TEXT("Entity: (%f, %f, %f) | WaterZ: %f | Depth: %f | WaterBody: %s"),
                    EntityLocation.X,
                    EntityLocation.Y,
                    EntityLocation.Z,
                    BestResult.GetWaterSurfaceLocation().Z,
                    BestResult.GetImmersionDepth(),
                    *BestBody->GetOwner()->GetActorLabel());
#endif

                // -----------------------------
                // -----------------------------
                constexpr float MinDepthToDestroy = 10.0f;
                if (BestDepth > MinDepthToDestroy)
                {
                    ChunkContext.Defer().DestroyEntity(ChunkContext.GetEntity(i));

#if WITH_EDITOR
                    UE_LOG(LogTemp, Warning, TEXT("Destroyed entity in lake %s"), *BestBody->GetOwner()->GetActorLabel());
#endif
                }
            }
        });

        // Rivers
        EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& ChunkContext)
            {
                auto Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();

                for (int32 i = 0; i < ChunkContext.GetNumEntities(); i++)
                {
                    const FVector EntityLocation = Transforms[i].GetTransform().GetLocation();

                    FWaterBodyQueryResult BestResult;
                    float BestDepth = -FLT_MAX;
                    UWaterBodyComponent* BestBody = nullptr;
                    FVector BestSplineLocation = FVector::ZeroVector;
                    float BestRiverHalfWidth = 0.0f;

                    for (UWaterBodyComponent* Body : CachedWaterBodies)
                    {
                        if (!Body || Body->GetWaterBodyType() != EWaterBodyType::River)
                            continue;

                        const FBox RiverBounds = Body->GetCollisionComponentBounds();
                        if (!RiverBounds.IsInsideXY(EntityLocation))
                            continue;

                        auto QueryResultOrError = Body->TryQueryWaterInfoClosestToWorldLocation(
                            EntityLocation,
                            QueryFlags,
                            {}
                        );

                        if (!QueryResultOrError.HasValue())
                            continue;

                        const FWaterBodyQueryResult& Result = QueryResultOrError.GetValue();
                        if (!Result.IsInWater())
                            continue;

                        UWaterSplineComponent* Spline = Body->GetWaterSpline();
                        const UWaterSplineMetadata* Meta = Body->GetWaterSplineMetadata();
                        if (!Spline || !Meta)
                            continue;

                        // Use the spline function
                        float SplineKey = Spline->FindInputKeyClosestToWorldLocation(EntityLocation);
                        FVector ClosestPointOnSpline = Spline->GetLocationAtSplineInputKey(SplineKey, ESplineCoordinateSpace::World);

                        float RiverHalfWidth = Meta->RiverWidth.Eval(SplineKey, 0.0f) * 0.5f;
                        float LateralDistSqr = FVector::DistSquared2D(EntityLocation, ClosestPointOnSpline);
                        if (LateralDistSqr > FMath::Square(RiverHalfWidth))
                            continue;

                        float Depth = Result.GetImmersionDepth();
                        if (Depth > BestDepth)
                        {
                            BestDepth = Depth;
                            BestResult = Result;
                            BestBody = Body;
                            BestSplineLocation = ClosestPointOnSpline;
                            BestRiverHalfWidth = RiverHalfWidth;
                        }
                    }

                    if (!BestBody)
                        continue;

                    constexpr float MinDepthToDestroy = 10.0f;
                    if (BestDepth > MinDepthToDestroy)
                    {
                        ChunkContext.Defer().DestroyEntity(ChunkContext.GetEntity(i));

#if WITH_EDITOR
                        UE_LOG(LogTemp, Warning,
                            TEXT("Destroyed entity at (%f, %f, %f) in river %s | SurfaceZ: %f | Depth: %f | SplineLoc: (%f, %f, %f) | RiverHalfWidth: %f"),
                            EntityLocation.X, EntityLocation.Y, EntityLocation.Z,
                            *BestBody->GetOwner()->GetActorLabel(),
                            BestResult.GetWaterSurfaceLocation().Z,
                            BestDepth,
                            BestSplineLocation.X, BestSplineLocation.Y, BestSplineLocation.Z,
                            BestRiverHalfWidth
                        );
#endif
                    }
                }
            });

 }

void UEnemyNavMeshGroundingProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);

	// This runs on the Game Thread - safe for GetAllActorsOfClass
	if (UWorld* World = Owner.GetWorld())
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(World, AWaterBody::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (AWaterBody* WaterBody = Cast<AWaterBody>(Actor))
			{
				CachedWaterBodies.Add(WaterBody->GetWaterBodyComponent());
			}
		}
		bWaterCacheInitialized = true;
	}
}
