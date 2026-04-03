// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterBodyProximtyProcessor.h"

#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "NavigationSystem.h"
#include "MassExecutionContext.h"



#include "WaterSplineComponent.h"
#include <StatusEffectFragments.h>

void ApplyBurn(FMassExecutionContext& Context, const FMassEntityHandle& Handle, float Duration, float DamagePerTick, float TickInterval);

UWaterBodyProximtyProcessor::UWaterBodyProximtyProcessor()
{

    bAutoRegisterWithProcessingPhases = true;
    ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
    ExecutionFlags = (int32)EProcessorExecutionFlags::All;
}

void UWaterBodyProximtyProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.Initialize(EntityManager);

    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
    //EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);

    EntityQuery.RegisterWithProcessor(*this);
}

void UWaterBodyProximtyProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

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

                    const FBox LakeBounds = Body->GetCollisionComponentBounds();
                    if (!LakeBounds.IsInsideXY(EntityLocation))
                        continue; // Skip if entity is outside the XY bounds

                    // Optional: further restrict by distance to lake center
                    const FVector LakeCenter = LakeBounds.GetCenter();
                    const float DistSqr = FVector::DistSquared2D(EntityLocation, LakeCenter);
                    constexpr float MaxLakeDistance = 5000.0f; // adjust per lake size
                    if (DistSqr > FMath::Square(MaxLakeDistance))
                        continue;

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

                constexpr float MinDepthToDestroy = 10.0f;
                if (BestDepth > MinDepthToDestroy)
                {
                    //ChunkContext.Defer().DestroyEntity(ChunkContext.GetEntity(i));
                    ApplyBurn(ChunkContext, ChunkContext.GetEntity(i), 5.0f, 1.0f, 1.0f);

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
                    //ChunkContext.Defer().DestroyEntity(ChunkContext.GetEntity(i));
                    ApplyBurn(ChunkContext, ChunkContext.GetEntity(i), 5.0f, 1.0f, 1.0f);


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

void ApplyBurn(FMassExecutionContext& Context, const FMassEntityHandle& Handle, float Duration, float DamagePerTick, float TickInterval)
{
    Context.Defer().PushCommand<FMassDeferredSetCommand>(
        [Handle, Duration, DamagePerTick, TickInterval](FMassEntityManager& Manager)
        {
            if (!Manager.IsEntityValid(Handle)) return;

            FBurnFragment* Burn = Manager.GetFragmentDataPtr<FBurnFragment>(Handle);
            if (Burn)
            {
                // Refresh duration
                Burn->Duration = FMath::Max(Burn->Duration, Duration);

                // Strongest damage wins (change if you want stacking)
                Burn->DamagePerTick = FMath::Max(Burn->DamagePerTick, DamagePerTick);

                // Faster tick rate wins
                Burn->TickInterval = FMath::Min(Burn->TickInterval, TickInterval);

                // Prevent delaying next tick
                Burn->TimeToNextTick = FMath::Min(Burn->TimeToNextTick, TickInterval);
            }
            else
            {
                Manager.AddFragmentToEntity(Handle, FBurnFragment::StaticStruct(),
                    [Duration, DamagePerTick, TickInterval](void* Fragment, const UScriptStruct&)
                    {
                        auto* Frag = static_cast<FBurnFragment*>(Fragment);
                        Frag->Duration = Duration;
                        Frag->DamagePerTick = DamagePerTick;
                        Frag->TickInterval = TickInterval;
                        Frag->TimeToNextTick = TickInterval;
                    });
            }
        });
}

void UWaterBodyProximtyProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager)
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
