// Fill out your copyright notice in the Description page of Project Settings.

#include "StatusEffectProcessor.h"
#include "StatusEffectFragments.h"
#include "EnemyDamageMassProcessor.h"
#include "EnemyManagerSubsystem.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassRepresentationSubsystem.h"
#include "MassNavigationFragments.h"


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
	PyroclasticQuery.Initialize(EntityManager);
	PyroclasticQuery.AddRequirement<FPyroclasticFragment>(EMassFragmentAccess::ReadWrite);
	PyroclasticQuery.RegisterWithProcessor(*this);

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
	BurnQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadOnly);
	BurnQuery.RegisterWithProcessor(*this);

	// Only runs on entities currently being suppressed
	SuppressedQuery.Initialize(EntityManager);
	SuppressedQuery.AddRequirement<FSuppressedFragment>(EMassFragmentAccess::ReadWrite);
	SuppressedQuery.RegisterWithProcessor(*this);

	// Only runs on entities with active compounding injury stacks
	CompoundingInjuryQuery.Initialize(EntityManager);
	CompoundingInjuryQuery.AddRequirement<FCompoundingInjuryFragment>(EMassFragmentAccess::ReadWrite);
	CompoundingInjuryQuery.RegisterWithProcessor(*this);

	// Only runs on entities with an active conduit marker
	ConduitMarkerQuery.Initialize(EntityManager);
	ConduitMarkerQuery.AddRequirement<FConduitMarkerFragment>(EMassFragmentAccess::ReadWrite);
	ConduitMarkerQuery.RegisterWithProcessor(*this);
}

void UStatusEffectProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	// --- Speed effects (stun > slow > base) ---
	UWorld* World = GetWorld();
	SpeedEffectQuery.ForEachEntityChunk(Context, [DeltaTime, World](FMassExecutionContext& Context)
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
				MoveList[i].CreateNewAction(EMassMovementAction::Stand, *World);
				MoveList[i].DesiredSpeed.Set(0.f);
				Stun.Duration -= DeltaTime;
				if (Stun.Duration <= 0.f)
				{
					MoveList[i].CreateNewAction(EMassMovementAction::Move, *World);
					MoveList[i].DesiredSpeed.Set(BaseSpeed);
					Context.Defer().RemoveFragment<FStunFragment>(Context.GetEntity(i));
				}
			}
			else if (bHasSlow)
			{
				FSlowFragment& Slow = SlowList[i];
				MoveList[i].DesiredSpeed.Set(BaseSpeed * Slow.SpeedMultiplier);
				Slow.Duration -= DeltaTime;
				if (Slow.Duration <= 0.f)
				{
					MoveList[i].DesiredSpeed.Set(BaseSpeed);
					Context.Defer().RemoveFragment<FSlowFragment>(Context.GetEntity(i));
				}
			}
			else
			{
				MoveList[i].DesiredSpeed.Set(BaseSpeed);
			}
		}
	});

	// --- Pyroclastic shield recharge ---
	PyroclasticQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FPyroclasticFragment> PyroclasticList = Context.GetMutableFragmentView<FPyroclasticFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			FPyroclasticFragment& Pyro = PyroclasticList[i];
			if (Pyro.TimeToShield > 0.f)
			{
				Pyro.TimeToShield = FMath::Max(0.f, Pyro.TimeToShield - DeltaTime);
			}
		}
	});

	// --- Suppressing fire window tick ---
	SuppressedQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FSuppressedFragment> SuppressedList = Context.GetMutableFragmentView<FSuppressedFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			FSuppressedFragment& Suppressed = SuppressedList[i];
			Suppressed.RemainingTime -= DeltaTime;
			if (Suppressed.RemainingTime <= 0.f)
			{
				Context.Defer().RemoveFragment<FSuppressedFragment>(Context.GetEntity(i));
			}
		}
	});

	// --- Compounding injury window tick ---
	CompoundingInjuryQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FCompoundingInjuryFragment> CIList = Context.GetMutableFragmentView<FCompoundingInjuryFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			FCompoundingInjuryFragment& CI = CIList[i];
			CI.RemainingTime -= DeltaTime;
			if (CI.RemainingTime <= 0.f)
			{
				Context.Defer().RemoveFragment<FCompoundingInjuryFragment>(Context.GetEntity(i));
			}
		}
	});

	// --- Conduit marker duration tick ---
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	ConduitMarkerQuery.ForEachEntityChunk(Context, [DeltaTime, EnemyManager](FMassExecutionContext& Context)
	{
		TArrayView<FConduitMarkerFragment> ConduitList = Context.GetMutableFragmentView<FConduitMarkerFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			FConduitMarkerFragment& Conduit = ConduitList[i];
			Conduit.Duration -= DeltaTime;
			if (Conduit.Duration <= 0.f)
			{
				const FMassEntityHandle Entity = Context.GetEntity(i);
				Context.Defer().RemoveFragment<FConduitMarkerFragment>(Entity);
				if (EnemyManager)
				{
					EnemyManager->NotifyConduitMarkerExpired(Entity);
				}
			}
		}
	});

	// --- Burn (DoT duration tick) ---
	BurnQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FBurnFragment>          BurnList   = Context.GetMutableFragmentView<FBurnFragment>();
		TConstArrayView<FHealthFragment>   HealthList = Context.GetFragmentView<FHealthFragment>();
		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			FBurnFragment& Burn = BurnList[i];
			const FMassEntityHandle Entity = Context.GetEntity(i);

			Burn.Duration      -= DeltaTime;
			Burn.TimeToNextTick -= DeltaTime;

			if (Burn.TimeToNextTick <= 0.f)
			{
				const float Overflow = -Burn.TimeToNextTick;
				Burn.TimeToNextTick = FMath::Max(Burn.TickInterval - Overflow, 0.f);

				// Apply burn damage via FDamageFragment so resistances are respected
				Context.Defer().PushCommand<FMassDeferredSetCommand>(
					[Entity, Damage = Burn.DamagePerTick](FMassEntityManager& Manager)
					{
						if (!Manager.IsEntityValid(Entity)) return;
						FDamageFragment* Existing = Manager.GetFragmentDataPtr<FDamageFragment>(Entity);
						if (Existing)
						{
							Existing->DamageAmount += Damage;
						}
						else
						{
							Manager.AddFragmentToEntity(Entity, FDamageFragment::StaticStruct(),
								[Damage](void* Fragment, const UScriptStruct&)
								{
									auto* Frag = static_cast<FDamageFragment*>(Fragment);
									Frag->DamageAmount = Damage;
									Frag->DamageType   = EDamageType::Neutral;
								});
						}
					});
			}

			if (Burn.Duration <= 0.f)
			{
				Context.Defer().RemoveFragment<FBurnFragment>(Entity);
			}
		}
	});
}
