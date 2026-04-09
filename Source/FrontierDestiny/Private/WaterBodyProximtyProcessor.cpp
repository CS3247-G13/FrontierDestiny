// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterBodyProximtyProcessor.h"

#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "NavigationSystem.h"
#include "MassExecutionContext.h"



#include "WaterSplineComponent.h"
#include <StatusEffectFragments.h>

void ApplyBurn(FMassExecutionContext& Context, const FMassEntityHandle& Handle, float Duration, float DamagePerTick, float TickInterval);

/*
FORCEINLINE bool PointInPolygon(const FVector2D& Point, const TArray<FVector2D>& Poly)
{
    bool bInside = false;

    for (int32 i = 0, j = Poly.Num() - 1; i < Poly.Num(); j = i++)
    {
        const FVector2D& A = Poly[i];
        const FVector2D& B = Poly[j];

        const bool bIntersect =
            ((A.Y > Point.Y) != (B.Y > Point.Y)) &&
            (Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y + KINDA_SMALL_NUMBER) + A.X);

        if (bIntersect)
            bInside = !bInside;
    }

    return bInside;
}*/

FORCEINLINE int32 GetWindingNumber(const FVector2D& Point, const TArray<FVector2D>& Poly)
{
    int32 WindingNumber = 0;
    const int32 NumPoints = Poly.Num();

    for (int32 i = 0; i < NumPoints; i++)
    {
        const FVector2D& V1 = Poly[i];
        const FVector2D& V2 = Poly[(i + 1) % NumPoints];

        // Cross product logic: Is Point to the left of the line V1 -> V2?
        // (V2.x - V1.x) * (Point.y - V1.y) - (V2.y - V1.y) * (Point.x - V1.x)
        const float IsLeft = FVector2D::CrossProduct(V2 - V1, Point - V1);

        if (V1.Y <= Point.Y)
        {
            // Upward crossing: edge must cross above Point.Y AND Point must be to the left
            if (V2.Y > Point.Y && IsLeft > 0.0f)
            {
                WindingNumber++;
            }
        }
        else
        {
            // Downward crossing: edge must cross below Point.Y AND Point must be to the right
            // Note: In Winding Number, "is right" is IsLeft < 0
            if (V2.Y <= Point.Y && IsLeft < 0.0f)
            {
                WindingNumber--;
            }
        }
    }
    return WindingNumber;
}


FORCEINLINE bool PointInPolygon(const FVector2D& Point, const TArray<FVector2D>& Poly)
{
    return GetWindingNumber(Point, Poly) != 0;
}




static void BuildLakePolygon(UWaterSplineComponent* Spline, TArray<FVector2D>& OutPolygon)
{
    if (!Spline) return;

    const int32 NumPoints = Spline->GetNumberOfSplinePoints();
    if (NumPoints < 2) return;

    const int32 SamplesPerSegment = 8; // fixed sampling

    // Step 1: Use a temporary array for all points
    TArray<FVector2D> TempPolygon;
    TempPolygon.Reserve(NumPoints * SamplesPerSegment + 1);

    for (int32 i = 0; i < NumPoints; i++)
    {
        const float StartKey = static_cast<float>(i);
        const float EndKey = (i + 1 == NumPoints) ? 0.f : static_cast<float>(i + 1);

        for (int32 s = 0; s < SamplesPerSegment; s++)
        {
            const float Alpha = static_cast<float>(s) / SamplesPerSegment;
            const float Key = FMath::Lerp(StartKey, EndKey, Alpha);

            const FVector Pos = Spline->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World);
            const FVector2D Point(Pos.X, Pos.Y);

            // Avoid consecutive duplicates
            if (TempPolygon.Num() == 0 || !TempPolygon.Last().Equals(Point, 1.0f))
            {
                TempPolygon.Add(Point);
            }
        }
    }

    /*
    // Ensure closed polygon safely
    if (TempPolygon.Num() > 1)
    {
        FVector2D FirstPoint = TempPolygon[0]; // make a copy
        if (!TempPolygon.Last().Equals(FirstPoint, 1.0f))
        {
            TempPolygon.Add(FirstPoint);
        }
    }
    */

    // Step 2: Move the fully built array into OutPolygon
    OutPolygon = MoveTemp(TempPolygon);
}

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
                const FVector2D Point(EntityLocation.X, EntityLocation.Y);

                FWaterBodyQueryResult BestResult;
                float BestDepth = -FLT_MAX;
                UWaterBodyComponent* BestBody = nullptr;

                for (const FLakeData& Lake : CachedLakes)
                {
                    // 1. Early reject using bounds
                    if (Point.X < Lake.BoundsMin.X || Point.X > Lake.BoundsMax.X ||
                        Point.Y < Lake.BoundsMin.Y || Point.Y > Lake.BoundsMax.Y)
                    {
                        continue;
                    }

                    // 2. Check polygon
                    if (!PointInPolygon(Point, Lake.Polygon))
                        continue;

                    // 3. Safe water query
                    UWaterBodyComponent* Body = Lake.Body.Get();
                    if (!Body)
                        continue;

                    auto Query = Body->TryQueryWaterInfoClosestToWorldLocation(
                        EntityLocation,
                        QueryFlags,
                        {}
                    );

                    if (!Query.HasValue() || !Query.GetValue().IsInWater())
                        continue;

                    const float Depth = Query.GetValue().GetImmersionDepth();
                    if (Depth > BestDepth)
                    {
                        BestDepth = Depth;
                        BestResult = Query.GetValue();
                        BestBody = Body;
                    }
                }

                // Apply burn if valid lake found
                if (BestBody && BestDepth > 10.0f)
                {
                    ApplyBurn(ChunkContext, ChunkContext.GetEntity(i), 5.0f, 1.0f, 1.0f);

#if WITH_EDITOR
                    UE_LOG(LogTemp, Warning,
                        TEXT("Lake hit: Entity (%f, %f, %f) | WaterZ: %f | Depth: %f | Lake: %s"),
                        EntityLocation.X,
                        EntityLocation.Y,
                        EntityLocation.Z,
                        BestResult.GetWaterSurfaceLocation().Z,
                        BestDepth,
                        *BestBody->GetOwner()->GetActorLabel());
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

    if (UWorld* World = Owner.GetWorld())
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, AWaterBody::StaticClass(), FoundActors);

        CachedLakes.Empty();
        CachedWaterBodies.Empty();

        for (AActor* Actor : FoundActors)
        {
            if (AWaterBody* WaterBodyActor = Cast<AWaterBody>(Actor))
            {
                UWaterBodyComponent* Body = WaterBodyActor->GetWaterBodyComponent();
                if (!Body) continue;

                // Cache all bodies for rivers
                CachedWaterBodies.Add(Body);

                if (Body->GetWaterBodyType() != EWaterBodyType::Lake)
                    continue;

                UWaterSplineComponent* Spline = Body->GetWaterSpline();
                if (!Spline) continue;

                FLakeData LakeData;
                LakeData.Body = Body;

                // Build lake polygon safely
                BuildLakePolygon(Spline, LakeData.Polygon);

                // Compute bounds for fast rejection
                LakeData.InitializeBounds();

                CachedLakes.Add(MoveTemp(LakeData));

#if WITH_EDITOR
                UE_LOG(LogTemp, Warning,
                    TEXT("Cached lake: %s | Points: %d | BoundsMin=(%f,%f) | BoundsMax=(%f,%f)"),
                    *Body->GetOwner()->GetActorLabel(),
                    LakeData.Polygon.Num(),
                    LakeData.BoundsMin.X, LakeData.BoundsMin.Y,
                    LakeData.BoundsMax.X, LakeData.BoundsMax.Y);
#endif
            }
        }

        bWaterCacheInitialized = true;
    }
}