#pragma once
#include "WaterBodyActor.h"      // For AWaterBody
#include "WaterBodyComponent.h"  // For UWaterBodyComponent
#include "WaterSubsystem.h"      // If you use the subsystem
//#include "LakeData.generated.h"


struct FLakeData
{
   
    TArray<FVector2D> Polygon;

    FVector2D BoundsMin;

    FVector2D BoundsMax;

    TWeakObjectPtr<UWaterBodyComponent> Body;

    bool bIsLava;

    FLakeData()
        : BoundsMin(ForceInitToZero)
        , BoundsMax(ForceInitToZero)
        //, Body(nullptr)
    {
    }

    void InitializeBounds()
    {
        if (Polygon.Num() > 0)
        {
            BoundsMin = Polygon[0];
            BoundsMax = Polygon[0];
            for (const FVector2D& P : Polygon)
            {
                BoundsMin.X = FMath::Min(BoundsMin.X, P.X);
                BoundsMin.Y = FMath::Min(BoundsMin.Y, P.Y);
                BoundsMax.X = FMath::Max(BoundsMax.X, P.X);
                BoundsMax.Y = FMath::Max(BoundsMax.Y, P.Y);
            }
        }
    }
};