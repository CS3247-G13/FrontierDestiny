#include "MassAttackCoreTask.h"
#include "MassCommonFragments.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "CoreManagerSubsystem.h"
#include "CoreActor.h"
#include "EnemyManagerSubsystem.h"
#include "HordeIDFragment.h"
#include "EnemyDamageMassProcessor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassAttackCoreTask)

bool FMassAttackCoreTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(StatsHandle);
	return true;
}

EStateTreeRunStatus FMassAttackCoreTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassContext = static_cast<const FMassStateTreeExecutionContext&>(Context);
	UWorld* World = MassContext.GetEntityManager().GetWorld();
	if (!World) return EStateTreeRunStatus::Failed;

	UCoreManagerSubsystem* CoreManager = World->GetSubsystem<UCoreManagerSubsystem>();
	if (!CoreManager) return EStateTreeRunStatus::Failed;

	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	const FVector EntityLocation = TransformFragment.GetTransform().GetLocation();

	float Damage = 10.f;
	if (const FStatsFragment* Stats = Context.GetExternalDataPtr(StatsHandle))
	{
		Damage = Stats->BaseDamage;
	}

	UE_LOG(LogTemp, Warning, TEXT("MassAttackCoreTask: EnterState called, damage=%.1f"), Damage);

	if (ACoreActor* Core = CoreManager->GetNearestActiveCore(EntityLocation))
	{
		Core->ApplyDamage(Damage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MassAttackCoreTask: No active core found"));
	} 

	const FMassEntityHandle Entity = MassContext.GetEntity();

	FMassEntityManager& EntityManager = MassContext.GetEntityManager();

	if (FHealthFragment* Health = EntityManager.GetFragmentDataPtr<FHealthFragment>(Entity))
	{
		Health->Value = 0.0f;
	}

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Entity](FMassEntityManager& Manager)
		{
			if (!Manager.IsEntityValid(Entity)) return;
			Manager.AddFragmentToEntity(Entity, FDamageFragment::StaticStruct(),
				[](void* Fragment, const UScriptStruct&)
				{
					static_cast<FDamageFragment*>(Fragment)->DamageAmount = 9999.f;
				});
		});

	return EStateTreeRunStatus::Succeeded;
}
