// Fill out your copyright notice in the Description page of Project Settings.

#include "CacheManagerSubsystem.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

void UCacheManagerSubsystem::RegisterCache(AActor* Cache)
{
	if (IsValid(Cache))
		Caches.Add(Cache);
}

void UCacheManagerSubsystem::DeregisterCache(AActor* Cache)
{
	if (IsValid(Cache))
	{
		TArray<UPrimitiveComponent*> Components;
		Cache->GetComponents<UPrimitiveComponent>(Components);
		for (UPrimitiveComponent* Comp : Components)
			Comp->SetRenderCustomDepth(false);
	}

	Caches.Remove(Cache);
}

void UCacheManagerSubsystem::UpdateCacheHighlights(FVector PlayerPos, float Radius, bool bEnabled)
{
	const float RadiusSq = Radius * Radius;

	for (const TObjectPtr<AActor>& Cache : Caches)
	{
		if (!IsValid(Cache)) continue;

		const bool bInRange = bEnabled && FVector::DistSquared2D(Cache->GetActorLocation(), PlayerPos) <= RadiusSq;

		TArray<UPrimitiveComponent*> Components;
		Cache->GetComponents<UPrimitiveComponent>(Components);
		for (UPrimitiveComponent* Comp : Components)
		{
			Comp->SetRenderCustomDepth(bInRange);
			if (bInRange)
				Comp->SetCustomDepthStencilValue(37);
		}
	}
}

TArray<FVector> UCacheManagerSubsystem::GetCachesInRange(FVector PlayerPos, float Radius) const
{
	TArray<FVector> Result;
	const float RadiusSq = Radius * Radius;

	for (const TObjectPtr<AActor>& Cache : Caches)
	{
		if (!IsValid(Cache)) continue;
		const FVector Pos = Cache->GetActorLocation();
		if (FVector::DistSquared2D(Pos, PlayerPos) <= RadiusSq)
			Result.Add(Pos);
	}

	return Result;
}
