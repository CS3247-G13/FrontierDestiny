// Fill out your copyright notice in the Description page of Project Settings.

#include "MassCommonFragments.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include <MassRepresentationSubsystem.h>
#include "EnemyManagerSubsystem.h"
#include "StatusEffectFragments.h"

#include "HealthbarUpdateProcessor.h"

UHealthbarUpdateProcessor::UHealthbarUpdateProcessor()
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
	ExecutionOrder.ExecuteAfter.Add(UEnemyDamageMassProcessor::StaticClass()->GetFName());
}

void UHealthbarUpdateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);

	// Optional — present only on enemies with the respective modifier/effect
	EntityQuery.AddRequirement<FModifierFragment>(EMassFragmentAccess::ReadOnly,  EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FVitalityFragment>(EMassFragmentAccess::ReadOnly,  EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FBurnFragment>(EMassFragmentAccess::ReadOnly,      EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FSlowFragment>(EMassFragmentAccess::ReadOnly,      EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FStunFragment>(EMassFragmentAccess::ReadOnly,      EMassFragmentPresence::Optional);

	EntityQuery.RegisterWithProcessor(*this);
}

// Modifier flag bit positions — must match what the Niagara material expects
static constexpr int32 MF_Fast        = 1 << 0;
static constexpr int32 MF_Strong      = 1 << 1;
static constexpr int32 MF_Vitality    = 1 << 2;
static constexpr int32 MF_Armoured    = 1 << 3;
static constexpr int32 MF_Reflective  = 1 << 4;
static constexpr int32 MF_Insulated   = 1 << 5;
static constexpr int32 MF_Stealthy    = 1 << 6;
static constexpr int32 MF_Nimble      = 1 << 7;
static constexpr int32 MF_Pyroclastic = 1 << 8;
static constexpr int32 MF_Amorphic    = 1 << 9;
static constexpr int32 MF_Distorted   = 1 << 10;
static constexpr int32 MF_Fragmented  = 1 << 11;
static constexpr int32 MF_SpeedTier1  = 1 << 12;
static constexpr int32 MF_SpeedTier2  = 1 << 13;
static constexpr int32 MF_SpeedTier3  = 1 << 14;


void UHealthbarUpdateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI) return;
	UEnemyManagerSubsystem* Subsystem = GI->GetSubsystem<UEnemyManagerSubsystem>();
	if (!Subsystem) return;

	UEnemyManagerSubsystem::FHealthbarFrameData Data;
	Data.DeltaTime = GetWorld()->GetDeltaSeconds();

	EntityQuery.ForEachEntityChunk(Context, [&Data](FMassExecutionContext& Context)
	{
		TConstArrayView<FHealthFragment>   HealthList    = Context.GetFragmentView<FHealthFragment>();
		TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		TConstArrayView<FModifierFragment> ModifierList  = Context.GetFragmentView<FModifierFragment>();
		TConstArrayView<FVitalityFragment> VitalityList  = Context.GetFragmentView<FVitalityFragment>();
		TConstArrayView<FBurnFragment>     BurnList       = Context.GetFragmentView<FBurnFragment>();
		TConstArrayView<FSlowFragment>     SlowList       = Context.GetFragmentView<FSlowFragment>();
		TConstArrayView<FStunFragment>     StunList       = Context.GetFragmentView<FStunFragment>();

		const bool bHasModifiers = !ModifierList.IsEmpty();
		const bool bHasVitality  = !VitalityList.IsEmpty();
		const bool bHasBurn      = !BurnList.IsEmpty();
		const bool bHasSlow      = !SlowList.IsEmpty();
		const bool bHasStun      = !StunList.IsEmpty();
		const int32 NumEntities  = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; i++)
		{
			const FHealthFragment& Health = HealthList[i];

			Data.Handles.Add(Context.GetEntity(i));
			Data.Positions.Add(TransformList[i].GetTransform().GetLocation());
			Data.Healths.Add(Health.Value);
			Data.MaxHealths.Add(Health.MaxValue);

			float Vitality = 0.f;
			if (bHasVitality)
			{
				Vitality = VitalityList[i].Value;
			}
			Data.Vitalities.Add(Vitality);

			// Status effect durations
			Data.BurnDurations.Add(bHasBurn ? BurnList[i].Duration : 0.f);
			Data.SlowDurations.Add(bHasSlow ? SlowList[i].Duration : 0.f);
			Data.StunDurations.Add(bHasStun ? StunList[i].Duration : 0.f);

			// Modifier flags packed as int cast to float
			int32 Flags = 0;
			if (bHasModifiers)
			{
				const FModifierFragment& Mod = ModifierList[i];
				if (Mod.bFast)        Flags |= MF_Fast;
				if (Mod.bStrong)      Flags |= MF_Strong;
				if (Mod.bVitality)    Flags |= MF_Vitality;
				if (Mod.bArmoured)    Flags |= MF_Armoured;
				if (Mod.bReflective)  Flags |= MF_Reflective;
				if (Mod.bInsulated)   Flags |= MF_Insulated;
				if (Mod.bStealthy)    Flags |= MF_Stealthy;
				if (Mod.bNimble)      Flags |= MF_Nimble;
				if (Mod.bPyroclastic) Flags |= MF_Pyroclastic;
				if (Mod.bAmorphic)    Flags |= MF_Amorphic;
				if (Mod.bDistorted)   Flags |= MF_Distorted;
				if (Mod.bFragmented)  Flags |= MF_Fragmented;
			}
			if (bHasModifiers && ModifierList[i].bDistorted)
			{
				const float HealthRatio = HealthList[i].Value / HealthList[i].MaxValue;
				if      (HealthRatio < 0.25f) Flags |= MF_SpeedTier3;
				else if (HealthRatio < 0.50f) Flags |= MF_SpeedTier2;
				else if (HealthRatio < 0.75f) Flags |= MF_SpeedTier1;
			}

			Data.ModifierFlags.Add((float)Flags);
			Data.FragmentedChunkSizes.Add(bHasModifiers ? ModifierList[i].FragmentedChunkSize : 0.f);
		}
	});

	Subsystem->UpdateHealthbarInformation(Data);
}
