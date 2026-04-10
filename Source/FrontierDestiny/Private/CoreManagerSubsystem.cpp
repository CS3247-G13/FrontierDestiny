// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreManagerSubsystem.h"

void UCoreManagerSubsystem::RegisterCore(ACoreActor* Core)
{
	if (!Core)
	{
		return;
	}

	CoreMap.Add(Core->CoreIndex, Core);

	Core->OnCoreActivated.AddDynamic(this, &UCoreManagerSubsystem::HandleCoreActivated);
	Core->OnCoreDestroyed.AddDynamic(this, &UCoreManagerSubsystem::HandleCoreDestroyed);
}

void UCoreManagerSubsystem::ActivateCore(int32 CoreIndex)
{
	if (ACoreActor** Found = CoreMap.Find(CoreIndex))
	{
		(*Found)->ActivateCore();
		isCoreCaptured[CoreIndex] = true;
	}
}

bool UCoreManagerSubsystem::IsCoreCaptured(int32 CoreIndex) const
{
	return isCoreCaptured[CoreIndex];
}

int32 UCoreManagerSubsystem::GetCapturedCoreCount() const
{
	return CapturedCoreCount;
}

ACoreActor* UCoreManagerSubsystem::GetCore(int32 CoreIndex) const
{
	const ACoreActor* const* Found = CoreMap.Find(CoreIndex);
	return Found ? const_cast<ACoreActor*>(*Found) : nullptr;
}

FVector UCoreManagerSubsystem::GetNearestActiveCoreLocation(FVector FromLocation) const
{
	float BestDistSq = FLT_MAX;
	FVector BestLocation = FVector::ZeroVector;

	for (const auto& Pair : CoreMap)
	{
		const ACoreActor* Core = Pair.Value;
		if (!Core || !Core->bIsCoreActive) continue;

		const float DistSq = FVector::DistSquared(FromLocation, Core->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestLocation = Core->GetActorLocation();
		}
	}

	return BestLocation;
}

ACoreActor* UCoreManagerSubsystem::GetNearestActiveCore(FVector FromLocation) const
{
	float BestDistSq = FLT_MAX;
	ACoreActor* BestCore = nullptr;

	for (const auto& Pair : CoreMap)
	{
		ACoreActor* Core = Pair.Value;
		if (!Core || !Core->bIsCoreActive) continue;

		const float DistSq = FVector::DistSquared(FromLocation, Core->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestCore = Core;
		}
	}

	return BestCore;
}

void UCoreManagerSubsystem::HandleCoreActivated(ACoreActor* Core)
{
	CapturedCoreCount++;
	OnCapturedCoreCountChanged.Broadcast(CapturedCoreCount);
}

void UCoreManagerSubsystem::HandleCoreDestroyed(ACoreActor* Core)
{
	// A destroyed core was active — decrement the captured count.
	if (Core && Core->bIsCoreActive)
	{
		CapturedCoreCount = FMath::Max(0, CapturedCoreCount - 1);
		OnCapturedCoreCountChanged.Broadcast(CapturedCoreCount);
	}
}
