// Fill out your copyright notice in the Description page of Project Settings.

#include "StatusEffectProcessor.h"
#include "StatusEffectFragments.h"
#include "EnemyDamageMassProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassRepresentationSubsystem.h"
#include "MassNavigationFragments.h"

static constexpr float SlowSpeedMultiplier = 0.8f; // 20% slow per GDD

UStatusEffectProcessor::UStatusEffectProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client |
		EProcessorExecutionFlags::Standalone |
		EProcessorExecutionFlags::Editor);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
	ExecutionOrder.ExecuteAfter.Add(UEnemyDamageMassProcessor::StaticClass()->GetFName());
}

void UStatusEffectProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// Runs on any entity with base stats and a move target (regardless of active effects)
	SpeedEffectQuery.Initialize(EntityManager);
	SpeedEffectQuery.AddRequirement<FStatsFragment>(EMassFragmentAccess::ReadOnly);
	SpeedEffectQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	SpeedEffectQuery.AddRequirement<FSlowFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	SpeedEffectQuery.AddRequirement<FStunFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	SpeedEffectQuery.RegisterWithProcessor(*this);

	// Only runs on entities that are currently burning
	BurnQuery.Initialize(EntityManager);
	BurnQuery.AddRequirement<FBurnFragment>(EMassFragmentAccess::ReadWrite);
	BurnQuery.RegisterWithProcessor(*this);
}

void UStatusEffectProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	// --- Speed effects (stun > slow > base) ---
	SpeedEffectQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TConstArrayView<FStatsFragment>     StatsList = Context.GetFragmentView<FStatsFragment>();
		TArrayView<FMassMoveTargetFragment> MoveList  = Context.GetMutableFragmentView<FMassMoveTargetFragment>();
		TArrayView<FSlowFragment>           SlowList  = Context.GetMutableFragmentView<FSlowFragment>();
		TArrayView<FStunFragment>           StunList  = Context.GetMutableFragmentView<FStunFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		const bool bHasSlow = !SlowList.IsEmpty();
		const bool bHasStun = !StunList.IsEmpty();

		for (int32 i = 0; i < NumEntities; i++)
		{
			const float BaseSpeed = StatsList[i].BaseSpeed;

			if (bHasStun)
			{
				FStunFragment& Stun = StunList[i];
				MoveList[i].DesiredSpeed.Set(0.f);
				Stun.Duration -= DeltaTime;
				if (Stun.Duration <= 0.f)
				{
					MoveList[i].DesiredSpeed.Set(BaseSpeed);
					Context.Defer().RemoveFragment<FStunFragment>(Context.GetEntity(i));
				}
			}
			else if (bHasSlow)
			{
				FSlowFragment& Slow = SlowList[i];
				MoveList[i].DesiredSpeed.Set(BaseSpeed * SlowSpeedMultiplier);
				Slow.Duration -= DeltaTime;
				if (Slow.Duration <= 0.f)
				{
					MoveList[i].DesiredSpeed.Set(BaseSpeed);
					Context.Defer().RemoveFragment<FSlowFragment>(Context.GetEntity(i));
				}
			}
			else
			{
				// Log unmodified DesiredSpeed to see what the nav system sets it to
				const FMassEntityHandle Entity = Context.GetEntity(i);
				UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] Entity(%d,%d) no effect -> DesiredSpeed=%.2f (BaseSpeed=%.2f)"),
					Entity.Index, Entity.SerialNumber, MoveList[i].DesiredSpeed.Get(), BaseSpeed);
			}
		}
	});

	// --- Burn (DoT duration tick) ---
	BurnQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FBurnFragment> BurnList = Context.GetMutableFragmentView<FBurnFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			BurnList[i].Duration -= DeltaTime;
			if (BurnList[i].Duration <= 0.f)
			{
				Context.Defer().RemoveFragment<FBurnFragment>(Context.GetEntity(i));
			}
		}
	});
}
