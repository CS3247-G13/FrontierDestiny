// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerUtilities.h"

FRotator UTowerUtilities::QuaternionSlerp(FRotator From, FRotator To, float DeltaTime, float RotationalSpeed)
{
    FQuat StartQuat(From);
    FQuat TargetQuat(To);

    // 1. Find the angular distance (in Radians) between the two quaternions
    float AngularDistance = StartQuat.AngularDistance(TargetQuat);

    // 2. If we are already there, or the distance is negligible, just return the target
    if (AngularDistance < KINDA_SMALL_NUMBER)
    {
        return To;
    }

    // 3. Calculate how many Radians we can move this frame
    // RotationalSpeed is likely in Degrees, so we convert it to Radians
    float MaxRadiansThisFrame = FMath::DegreesToRadians(RotationalSpeed) * DeltaTime;

    // 4. Calculate Alpha based on the constant speed
    // This ensures we move at exactly 'RotationalSpeed' degrees per second
    float Alpha = FMath::Min(MaxRadiansThisFrame / AngularDistance, 1.0f);

    // 5. Perform the Slerp and return
    return FQuat::Slerp(StartQuat, TargetQuat, Alpha).Rotator();
}